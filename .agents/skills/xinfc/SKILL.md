---
name: xinfc-release
description: Release workflow for XiNFC OpenWrt packages. Use when bumping PKG_VERSION in xinfc or luci-app-xinfc Makefiles, creating a release tag, triggering CI build, or updating version references across the repo. Also use when the user mentions "release", "bump version", "tag", or "PKG_VERSION".
---

# xinfc release workflow

## Bump version

See [docs/openwrt-versioning.md](docs/openwrt-versioning.md) for the
semantics of PKG_VERSION vs PKG_RELEASE.

- `PKG_VERSION` — every code change bumps this
- `PKG_RELEASE` — always `1` (we control the code, `-r1` lets install.sh hardcode the URL)

Change both Makefiles:

- `package/xinfc/Makefile` — bump PKG_VERSION, keep PKG_RELEASE=1
- `package/luci-app-xinfc/Makefile` — bump both versions together, reset PKG_RELEASE to 1

## Build debug

CI runs in `openwrt/sdk:mediatek-filogic-<version>` where `<version>` is the matrix from `build.yml` (currently `v24.10.7`, `v25.12.4`). The Makefile inline `define Package/xinfc/postinst` uses `$$$${IPKG_INSTROOT}` (four `$`) — GNU Make's `$(eval ...)` consumes one `$$` → `$`, recipe consumes another `$$` → `$`, final shell sees `${IPKG_INSTROOT}`.
