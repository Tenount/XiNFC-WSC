# Contributing to XiNFC

## Requirements

- [just](https://just.systems/) (task runner)
- [pre-commit](https://pre-commit.com/) (git hooks)
- [Docker or Podman](https://podman.io/) — linters and tests run in containers
- [drift](https://github.com/fiberplane/drift) (docs freshness check)

For the full list of linters and their versions see [docs/linter-suppressions.md](docs/linter-suppressions.md).

## Quick Commands

| Command | What it does |
|---------|-------------|
| `just lint` | Run all linters via MegaLinter (read-only — fails on findings) |
| `just fix` | Auto-fix all fixable issues (redundant declarations, format, etc.) |
| `just fmt` | Auto-format only (quick: clang-format, shfmt, prettier) |
| `just test` | Smoke + unit tests in Alpine container |
| `just drift` | Check docs are up to date |
| `just hooks` | Install git hooks (one-time) |
| `just check` | lint + drift + test |

## Commit Message Format

We follow [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

```text
<type>(<scope>): <description>
```

| Type | When | Example |
|------|------|---------|
| `feat` | New feature | `feat(xinfc): add NFC read support` |
| `fix` | Bug fix | `fix(luci-app-xinfc): correct encryption label` |
| `docs` | Documentation | `docs: update changelog` |
| `style` | Formatting (no code change) | `style: apply clang-format` |
| `refactor` | Code restructuring | `refactor(xinfc): simplify NDEF builder` |
| `test` | Adding tests | `test: add smoke test for binary` |
| `chore` | Build/CI/tooling | `chore: update pre-commit hooks` |
| `ci` | CI/CD changes | `ci: add shellcheck step` |
| `build` | Build system changes | `build: fix Makefile for luci.mk` |

**Scopes:** `xinfc` (C binary), `luci-app-xinfc` (LuCI), `ci`, `docs`, or none.

### Commit message rules

- **Subject** ≤ 72 characters
- **Description** lines ≤ 75 characters
- **Blank line** between subject and description
- **Description** starts with lowercase, no period at end
- Enforced by `commitlint` via pre-commit hook

## Versioning

[Semantic Versioning](https://semver.org/): `MAJOR.MINOR.PATCH`

- **MAJOR**: breaking changes (config format, API)
- **MINOR**: new features
- **PATCH**: bug fixes, docs, CI

Version is in `package/xinfc/Makefile` and `package/luci-app-xinfc/Makefile` (`PKG_VERSION`).

## Changelog

[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) format. See [CHANGELOG.md](CHANGELOG.md).

Categories: Added, Changed, Deprecated, Removed, Fixed, Security.

## Code Style

### C Code

- clang-format (LLVM style, InsertBraces, PointerAlignment)
- `-Wall -Wextra -Werror -Wshadow -Wconversion -Wformat=2 -Wpedantic`
- clang-tidy static analysis (fallible — all warnings fail CI)
- cppcheck static analysis

### Shell Scripts

- shellcheck clean
- `#!/bin/bash` when using `local` or bashisms

### JavaScript (LuCI)

- `form.JSONMap` + `view.extend` pattern
- See `package/luci-app-xinfc/htdocs/` for examples

## Pull Requests

1. Branch from `main`
2. Follow commit message format
3. Ensure `just check` passes (lint + drift + test)
4. Update CHANGELOG.md if user-facing change
5. Request review
