# OpenWrt Versioning: PKG_VERSION vs PKG_RELEASE

## Format

```makefile
PKG_VERSION:=1.2.0
PKG_RELEASE:=1
```

Result: `1.2.0-r1` (full version string)

## What each means

| Variable | Purpose | When to change |
|----------|---------|----------------|
| `PKG_VERSION` | Upstream software version | New software version (new features, bug fixes) |
| `PKG_RELEASE` | OpenWrt packaging revision | Makefile changes, patches, config tweaks (no code change) |

Combined in `include/package-defaults.mk`:

```makefile
VERSION = $(PKG_VERSION)$(if $(PKG_RELEASE),-r$(PKG_RELEASE))
```

If `PKG_RELEASE` is not defined → version shows without `-r` suffix.

## Examples

### busybox (third-party package)

```makefile
PKG_VERSION:=1.38.0
PKG_RELEASE:=2
```

→ `1.38.0-r2` — busybox upstream is 1.38.0, OpenWrt packaging revised 2 times

### podkop (date-based)

```makefile
PKG_VERSION := 0.$(shell date +%d%m%Y)
PKG_RELEASE:=1
```

→ `0.20062026-r1` — every build = new date

### xinfc (we control upstream)

```makefile
PKG_VERSION:=1.2.0
PKG_RELEASE:=1
```

→ `1.2.0-r1` — each code change = new version, PKG_RELEASE stays 1

## When to increment PKG_RELEASE

1. **Makefile changed** (dependencies, install paths, build flags)
2. **Patch added/updated** (without upstream version change)
3. **Config changed** (default options, enabled features)

Do NOT increment PKG_RELEASE when changing code — bump PKG_VERSION instead.

## install.sh implications

```bash
# If PKG_RELEASE is always 1:
PKG_VERSION="${PKG_VERSION:-1.2.0}"
PKG_RELEASE="${PKG_RELEASE:-1}"
# Download: xinfc_1.2.0-r1_aarch64.apk ✓

# If PKG_RELEASE varies:
# install.sh can't predict it → download fails
# Solution: parse from GitHub API or keep PKG_RELEASE:=1
```

## Recommendation for xinfc

- `PKG_RELEASE:=1` — always
- Every code change → bump `PKG_VERSION`
- install.sh can safely hardcode `-r1`
