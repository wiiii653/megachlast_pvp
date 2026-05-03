# AGENTS.md

Repository guidance for planning and coding agents working on Megachlast PvP.

## Repo Context

- This is a C++/SFML 3 project.
- Main build flow is CMake with a `build_sfml3` directory.
- The game runs from the project root with `assets/` in the current working directory.
- The repo may already contain unrelated local edits. Do not revert or overwrite them unless the user explicitly asks.
- Vendored SFML lives under `SFML-3.0.1/`; treat it as third-party code unless the task explicitly targets it.

## Planning Agent

- Read the repo first: `README.md`, the relevant source files, and the current git state.
- Identify the smallest set of files that need to change.
- Call out uncertain assumptions before implementation starts.
- Prefer concrete verification steps over vague plans.
- Keep the plan aligned with the existing code structure instead of introducing new abstractions early.

## Coding Agent

- Keep edits tightly scoped to the requested behavior.
- Preserve existing style and project patterns.
- Use `apply_patch` for manual file edits.
- Do not use destructive git commands.
- Do not touch unrelated working tree changes.
- Use `rg` / `rg --files` for discovery before falling back to slower tools.

## Build And Test

Use the project’s documented build flow:

```bash
cmake -S . -B build_sfml3
cmake --build build_sfml3 -j
```

Useful checks:

```bash
ctest --test-dir build_sfml3 --output-on-failure
```

If a change affects runtime behavior, verify the executable from the repo root so `assets/` resolves correctly.

## Working Rules

- Prefer local helpers and existing modules over new utility layers.
- Keep comments short and only where the code is not self-explanatory.
- Keep responses and commit messages factual and concise.
- When documenting results, include the actual files changed and the verification performed.
