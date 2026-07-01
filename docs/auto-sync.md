# Auto-sync NFC on Wi-Fi Credential Change

When the Wi-Fi password or SSID is changed, the NFC chip can be
updated automatically so phone taps always have current credentials.

## Components

- `/etc/init.d/xinfc-sync` — procd service that monitors `/etc/config/wireless`
  for changes via `procd_add_reload_trigger "wireless"`
- `/usr/share/xinfc/sync-handler.sh` — shared script that reads credentials
  from the configured Wi-Fi interface and writes them to the NFC chip
- `/etc/hotplug.d/iface/99-nfc-sync` — hotplug script for boot-time sync
  (triggers on `ifup`)

## Flow

```text
User changes SSID/password in LuCI → Network → Wireless → Save & Apply
↓
uci commit wireless writes /etc/config/wireless
↓
procd detects file change, calls /etc/init.d/xinfc-sync reload_service
↓
/usr/share/xinfc/sync-handler.sh runs
  checks auto_sync=1 in /etc/config/xinfc
  reads SSID/key/encryption from wireless.<sync_iface>
  passes encryption value directly (xinfc-wsc accepts OpenWrt names)
  writes to NFC chip via /usr/sbin/xinfc-wsc
  logs to syslog with tag "xinfc-nfc-sync"
```

## Configuration

```text
config nfc_config 'global'
    option iface 'default_radio0'   # Wi-Fi interface to sync from
    option auto_sync '1'            # Enable auto-sync (default: on)
```

Toggle via LuCI: **System → NFC Configuration → Auto-sync NFC when Wi-Fi credentials change**

## Debugging

```bash
logread | grep xinfc-nfc-sync    # Check sync logs
logread | grep xinfc-debug        # Debug logging (add temporarily)

# Run sync manually (as root):
/usr/share/xinfc/sync-handler.sh

# Test hotplug event:
ACTION=ifup INTERFACE=default_radio0 /etc/hotplug.d/iface/99-nfc-sync

# Init script:
/etc/init.d/xinfc-sync start
/etc/init.d/xinfc-sync enable
```

## Limitations

- Only the `iface` configured in `/etc/config/xinfc` is watched
- Pure WPA3 (SAE) is mapped to WPA2/WPA3 mixed (`sae-mixed`) because
  WSC has no SAE auth type bit
- The binary (`xinfc-wsc`) accepts all OpenWrt encryption names directly
  (see `g_openwrt_modes[]` in `wifi.c`). No normalization needed in scripts.
