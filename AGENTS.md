# AGENTS.md — AI Agent Context

## Project

**XiNFC-WSC** — OpenWrt package for writing Wi-Fi credentials to NFC chip on Xiaomi AX3000T.

## User-facing docs

- [README.md](README.md) — what the project does, installation, usage
- [CONTRIBUTING.md](CONTRIBUTING.md) — commit format, code style, PR workflow, dev setup
- [CHANGELOG.md](CHANGELOG.md) — release history

## Repository Structure

```text
XiNFC-WSC/
├── package/
│   ├── xinfc/               # C binary (I2C → NFC)
│   │   ├── Makefile           # OpenWrt build
│   │   ├── files/                # Shell scripts for installation
│   │   │   ├── clear-nfc.sh          # Clear NFC chip
│   │   │   ├── hotplug-nfc-sync.sh   # Auto-sync on Wi-Fi change
│   │   │   ├── sync-handler.sh       # Sync handler
│   │   │   ├── utils.sh              # Shared NFC functions
│   │   │   ├── write-admin.sh        # Admin URL writer
│   │   │   ├── write-text.sh         # Write text NDEF record
│   │   │   ├── write-wifi.sh         # Write Wi-Fi NDEF record
│   │   │   └── xinfc-sync.init       # Init script
│   │   ├── src/                    # C sources (9 files)
│   │   │   ├── main.c              # CLI parsing, lock handling, dispatch
│   │   │   ├── xinfc-core.c        # Core NFC operations (apply, read, write, scan)
│   │   │   ├── i2c_nfc_device.c/h  # I2C → NFC (ioctl, read/write, init helper)
│   │   │   ├── wifi.c/h            # Encryption modes + WSC constants
│   │   │   ├── version.h           # XINFC_VERSION macro fallback
│   │   │   └── xinfc-utils.c/h     # NDEF builders, parsers, utilities
│   └── luci-app-xinfc/      # LuCI interface (JS)
│       ├── Makefile                    # standard luci.mk build system
│       ├── htdocs/.../xinfc/xinfc.js  # View + save handler
│       ├── po/ru/xinfc.po             # Russian localization
├── tests/
│   ├── unit/                # cmocka unit tests (46 tests)
│   │   ├── Makefile         # Build with build_dir output
│   │   ├── test_wifi.c      # Tests for utils + wifi
│   │   └── test_lock.c      # Tests for flock-based locking
│   └── smoke/
│       └── test-binary.sh   # Binary smoke test
├── .github/workflows/       # CI: MegaLinter + build + SDK
├── .mega-linter.yml         # MegaLinter config (18 linters)
├── .semgrepignore           # Semgrep ignore patterns
├── docs/                    # Documentation
├── justfile                 # Dev commands (test, lint, build)
└── docker/                  # Custom MegaLinter image
```

## Stack

| Component | Technology |
|-----------|-----------|
| Backend | C (gcc, musl) |
| Frontend | LuCI JavaScript (form.JSONMap) |
| CI | GitHub Actions + custom MegaLinter image (verify only) |
| SAST | Semgrep with 73 cached C rules (r/c) |
| Unit test env | Alpine container (cmocka, gcc) |
| Build | OpenWrt SDK (mediatek/filogic) |
| Packages | .ipk (OpenWrt 24.x), .apk (OpenWrt 25.x) |

## Key Decisions

1. **Two packages**: `xinfc` (C binary) + `luci-app-xinfc` (LuCI JS) — same pattern as podkop
2. **luci.mk** — standard LuCI build system
3. **C, not C++** — saves ~200KB libstdc++ dependency
4. **MegaLinter c_cpp** — CI = gatekeeper, no auto-fix
5. **drift** — binds AGENTS.md to key files, pre-commit blocks stale docs

## CI Pipeline

```text
push to main → MegaLinter (verify only)
tag push v*.*.* → Build (SDK) → Release
```

## Code Patterns

- **Luci admin route**: `admin/system/xinfc` (canonical in `menu.d/luci-app-xinfc.json`)
- **LuCI JS**: `form.JSONMap` + `form.NamedSection('global')` for non-UCI forms
- **I2C init**: `i2c_init_device()` extracted helper, `i2c_make_device()` for low-level
- **Scan**: `scan_i2c_bus()` probes addresses 0x50–0x57 for debugging
- **Encryption**: `select_encryption_mode()` parses string → enum wifi_crypt + enum wifi_auth
- **CLI split**: `main.c` (getopt_long, lock) → `xinfc-core.c` (NFC ops)
- **UCI config**: `/etc/config/xinfc` — `nfc_config.global` section with `iface` option. Template in `root/etc/config/xinfc`, managed by LuCI. Comments use `#`. Config is preserved across package upgrades (conffile). See [UCI docs](https://openwrt.org/docs/techref/uci).

## Language

All comments in source code, commit messages, documentation, and CI config must be in **English**. This ensures accessibility for international contributors and AI agents.

## Reference Files

- [docs/linter-suppressions.md](docs/linter-suppressions.md) — suppressed rules and why
- [.mega-linter.yml](.mega-linter.yml) — linter configuration
- [.semgrepignore](.semgrepignore) — semgrep ignore patterns
- [justfile](justfile) — available dev commands

## Debugging

- **MegaLinter fails**: Check `.mega-linter.yml` ENABLE_LINTERS, verify linter config files exist. If clang-tidy fails with "Fatal error" — the custom MegaLinter image needs rebuilding: push to main triggers `megalinter-build.yml` automatically, or run `workflow_dispatch` manually.
- **drift reports stale**: Run `drift link --doc-is-still-accurate AGENTS.md <changed-file>`
- **Never skip hooks** with `--no-verify` — CI will fail on the same issues, creating a wasted round-trip. Run `just check` before any commit
- **CI fails on format**: Run `just fmt` locally, commit changes, push
- **cppcheck false positive**: Add `// cppcheck-suppress <rule>` with explanation
- **fs.exec Permission denied**: LuCI `fs.exec()` goes through ubus/rpcd. Any script called via `fs.exec()` must be listed in `package/luci-app-xinfc/root/usr/share/rpcd/acl.d/luci-app-xinfc.json` under `read.file`. Binary in `/usr/sbin/xinfc-wsc` is already allowed. Shell scripts (e.g. `write-admin.sh`) added later need an explicit ACL entry.
- **Build fails**: Check OpenWrt SDK version matches `OPENWRT_VERSION` in build.yml
- **Semgrep findings**: Rules cached in image at `/usr/local/share/semgrep-rules/c.yaml`. To update: rebuild image with `podman build -f docker/Dockerfile.megalinter -t megalinter-xinfc:latest .`
