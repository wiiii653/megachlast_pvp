---
name: analyse-project
description: Analyse an unfamiliar project and produce a structured status report covering architecture, build health, and known issues.
---

# Analyse Project

Systematically explore a codebase you haven't worked on before and produce a concise status report.

## When to use

- User asks to "analyse the project", "check the project", "review the codebase", or similar
- Starting work on an unfamiliar repo and need orientation
- Verifying project health after a period of inactivity

## Procedure

1. **Read project memory** — check `MEMORY.md` or checkpoint files for prior context
2. **Explore structure** — list root directory, read `README.md`, identify language/framework
3. **Map architecture** — find entry points, key modules, config files, build system
4. **Check build health** — run the project's build command (CMake, dotnet, make, etc.)
5. **Run tests** — execute the test suite if one exists
6. **Check git state** — `git status`, `git log --oneline -10`, branch info
7. **Identify issues** — compile errors, failing tests, TODO/FIXME comments, missing dependencies
8. **Produce report** — structured summary with sections below

## Report format

```markdown
## Project: <name>

### Overview
<1-2 sentence summary of what this project is>

### Architecture
<key directories, entry points, dependencies>

### Build status
<build command result, pass/fail>

### Test status
<test results, pass/fail, count>

### Git state
<current branch, recent commits, dirty files>

### Issues found
<numbered list of problems with file:line references>

### Recommendations
<prioritized next steps>
```

## Stopping condition

Report is complete when all sections are filled and the user has been presented with the summary.

## Notes

- Adapt the build/test commands to the project's actual toolchain
- If the project has no tests, note that explicitly
- Keep the report under 200 lines — favour bullet points over prose
- If the user reports a specific bug, investigate that after the general analysis
