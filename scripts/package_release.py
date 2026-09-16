"""Stage dependencies, verify the game, then create a platform archive."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def run(*args, **kwargs):
    return subprocess.check_output(args, text=True, **kwargs)


parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--platform', required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
build = args.build.resolve()
package = root / 'release' / ('megachlast-' + args.platform)
package.mkdir(parents=True, exist_ok=False)
windows = sys.platform == 'win32'
exe_name = 'megablast_pvp_sfml' + ('.exe' if windows else '')
exe = build / ('Release' if windows else '') / exe_name
target = package / exe_name
shutil.copy2(exe, target)
shutil.copytree(root / 'assets', package / 'assets', ignore=shutil.ignore_patterns('settings.cfg'))
for name in ('LICENSE', 'README.md', 'EARLY_ACCESS.md', 'CHANGELOG.md', 'KNOWN_ISSUES.md'):
    shutil.copy2(root / name, package / name)

licenses = package / 'licenses'
licenses.mkdir()
sfml = build / '_deps' / 'sfml-src'
for source_tree in (build / '_deps').glob('*-src'):
    label = 'SFML' if source_tree == sfml else source_tree.name.removesuffix('-src')
    for path in source_tree.rglob('*'):
        if path.is_file() and '.git' not in path.parts and any(
                word in path.name.lower() for word in ('license', 'copying', 'copyright', 'ftl.txt')):
            dest = licenses / label / path.relative_to(source_tree)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dest)
# These dependencies carry their license notices inside their source headers.
shutil.copytree(sfml / 'extlibs' / 'headers', licenses / 'SFML' / 'embedded-header-notices')
if not (licenses / 'SFML' / 'license.md').exists():
    raise RuntimeError('SFML license missing')

if sys.platform.startswith('linux'):
    lib = package / 'lib'
    lib.mkdir()
    # libc and the graphics driver stack must come from the host OS.
    system = re.compile(r'^(linux-vdso|ld-linux|lib(c|m|pthread|dl|rt|resolv)\.so|lib(GL|EGL|GLX|GLdispatch|OpenGL|drm))')
    deps = run('ldd', str(target))
    if 'not found' in deps:
        raise RuntimeError(deps)
    for line in deps.splitlines():
        match = re.match(r'\s*(\S+) => (/\S+)', line)
        if not match or system.match(match[1]):
            continue
        name, source = match.groups()
        shutil.copy2(source, lib / name)
        run('patchelf', '--set-rpath', '$ORIGIN', str(lib / name))
        owner = run('dpkg-query', '-S', str(Path(source).resolve())).split(': ')[0]
        doc = Path('/usr/share/doc') / owner.split(':')[0] / 'copyright'
        if not doc.exists():
            raise RuntimeError('Missing dependency copyright: ' + owner)
        shutil.copy2(doc, licenses / (owner.replace(':', '-') + '.txt'))
    run('patchelf', '--set-rpath', '$ORIGIN/lib', str(target))
elif sys.platform == 'darwin':
    run('dylibbundler', '-b', '-cd', '-x', str(target), '-d', str(package / 'lib'),
        '-p', '@executable_path/lib/')
    for binary in [target, *(package / 'lib').glob('*.dylib')]:
        for line in run('otool', '-L', str(binary)).splitlines()[1:]:
            dependency = line.strip().split(' (')[0]
            if not dependency.startswith(('/usr/lib/', '/System/Library/', '@executable_path/', '@loader_path/')):
                raise RuntimeError('Unbundled macOS dependency: ' + dependency)
        run('codesign', '--force', '--sign', '-', str(binary))
else:
    # Static SFML and static MSVC CRT; copy any extra runtime DLLs if present.
    for dll in exe.parent.glob('*.dll'):
        shutil.copy2(dll, package / dll.name)

if windows:
    launcher = package / 'play.bat'
    launcher.write_text('@echo off\ncd /d "%~dp0"\nmegablast_pvp_sfml.exe %*\n')
else:
    launcher = package / ('play.command' if sys.platform == 'darwin' else 'play.sh')
    launcher.write_text('#!/bin/sh\ncd -- "$(dirname -- "$0")" || exit 1\nexec ./megablast_pvp_sfml "$@"\n')
    launcher.chmod(0o755)

environment = dict(os.environ)
for key in ('LD_LIBRARY_PATH', 'DYLD_LIBRARY_PATH', 'DYLD_FALLBACK_LIBRARY_PATH'):
    environment.pop(key, None)
run(str(target), '--smoke-test', cwd=package, env=environment)
run(str(target), '--help', cwd=package, env=environment)
if sys.platform == 'darwin':
    # Keep the game folder intact when copied out of the read-only disk image.
    disk_root = package.parent / 'dmg-content'
    disk_root.mkdir()
    shutil.copytree(package, disk_root / package.name)
    output = str(package) + '.dmg'
    run('hdiutil', 'create', '-volname', 'Megachlast PvP', '-srcfolder',
        str(disk_root), '-format', 'UDZO', output)
    run('hdiutil', 'verify', output)
    with tempfile.TemporaryDirectory() as mount:
        run('hdiutil', 'attach', '-readonly', '-nobrowse', '-mountpoint', mount, output)
        try:
            mounted = Path(mount) / package.name
            run(str(mounted / exe_name), '--smoke-test', cwd=mounted, env=environment)
        finally:
            run('hdiutil', 'detach', mount)
else:
    archive_format = 'zip' if windows else 'gztar'
    output = shutil.make_archive(str(package), archive_format, package.parent, package.name)
print('Packaged:', output)
