---
name: openwrt-uci-defaults
description: OpenWrt UCI defaults reference. Use when creating or editing UCI-defaults scripts (/etc/uci-defaults), adding custom files to an OpenWrt image (buildroot files/ or image builder FILES=), writing first-boot configuration for an OpenWrt package, or guarding against config overwrites on sysupgrade. Also use when the user mentions uci-defaults, uci batch, IPKG_INSTROOT, postinst, or first-boot scripts.
---

# OpenWrt UCI defaults

Reference for `/etc/uci-defaults` — scripts that run once at first boot to preconfigure UCI.

## Execution

All scripts in `/etc/uci-defaults` run automatically at first boot:

- **exit 0** → script is deleted
- **exit non-zero** → script stays, re-runs on next boot until it exits 0

Live router: `/rom/etc/uci-defaults` holds the originals; `/etc/uci-defaults` is empty (scripts ran and were deleted).

Order is lexical by filename — prefix with a number for ordering (e.g. `99-custom` runs last).

## Adding scripts

Three paths, same destination:

| Path | Mechanism |
|------|-----------|
| **Package** | File in package's `files/etc/uci-defaults/` — gets installed like any other file |
| **Buildroot** | `<buildroot>/files/etc/uci-defaults/` — custom files for SDK builds |
| **Image builder** | `files/etc/uci-defaults/` + `make image FILES="files"` |

Scripts should NOT be executable.

## Guarding against overwrites

Scripts fire on every first boot (clean install *and* upgrade). To avoid clobbering user settings:

```sh
[ "$(uci -q get system.@system[0].zonename)" = "America/New York" ] && exit 0
```

Probe a key your script would set — if it already matches, exit 0 and get deleted.

## Idiom: custom settings

```
set network.lan.ipaddr='192.168.1.1'
set wireless.@wifi-device[0].disabled='0'
uci commit
```

Use `uci -q batch << EOI` for multi-line blocks. Supports `set`, `add`, `rename`, `delete`, `commit`:

```
rename firewall.@zone[0]='lan'
rename firewall.@zone[1]='wan'
add dhcp host
set dhcp.@host[-1].name='bellerophon'
```

## References

- [UCI defaults](https://openwrt.org/docs/guide-developer/uci-defaults)
- [Build system — Custom files](https://openwrt.org/docs/guide-developer/toolchain/use-buildsystem#custom_files)
- [Image builder — Custom files](https://openwrt.org/docs/guide-user/additional-software/imagebuilder#custom_files)
- [The UCI system](https://openwrt.org/docs/guide-user/base-system/uci)
