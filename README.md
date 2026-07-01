# XiNFC for OpenWrt

NFC configuration tool for Xiaomi AX3000T with OpenWrt.

## What is this?

XiNFC writes Wi-Fi credentials to the NFC chip on Xiaomi routers (AX3000T).
Users tap their phone and connect without typing passwords.

## Features

XiNFC is an OpenWrt-native alternative to the stock Xiaomi NFC implementation,
with significantly more capabilities:

| Feature | Xiaomi Stock | XiNFC (OpenWrt) |
|---------|-------------|-----------------|
| Wi-Fi NFC write | ✅ Automatic | ✅ Via LuCI + auto-sync |
| Network selection | ❌ Main only | ✅ Any interface (main/guest) |
| Auto-sync on Wi-Fi change | ❌ | ✅ Hotplug-based sync |
| Link / text records | ❌ | ✅ URI + RTD-Text |
| Open Source | ❌ | ✅ GPL-3.0 |
| Diagnostics | ❌ | ✅ I2C scan, version info |
| Web interface | ❌ | ✅ LuCI with i18n |

### Current Capabilities

- **Write Wi-Fi** — NFC tag with WSC (Wi-Fi Simple Configuration) record; one-tap connect on Android
- **Any network** — Choose main SSID or guest VLAN, not just the primary
- **Auto-sync** — Hotplug script watches `/etc/config/wireless` and rewrites NFC on password change
- **Clear chip** — 3 modes: empty NDEF, text note, or full erase
- **Write text note** — Write a plain text NDEF record (`--write-text`)
- **Write URL** — Write a URL/link NDEF record (`--write-url`)
- **Admin URL** — One-click writes LuCI admin URL to NFC
- **Read tag** — View current NFC contents in LuCI or via CLI (`xinfc-wsc --read`)
- **I2C scan** — Probe bus for NFC chip address (debugging)

## Supported Devices

- Xiaomi AX3000T (MediaTek MT7981B / filogic)

## Supported OpenWrt Versions

- OpenWrt 24.10.x
- OpenWrt 25.12.x

Builds for both are published with each release.

### Requirements

- **Android device with NFC** — iOS does not support reading NFC tags via third-party apps
- NFC must be enabled on the Android device
- Screen must be unlocked when tapping the router

## Installation

### Quick Install (Recommended)

```bash
ssh root@192.168.1.1
sh <(wget -O - https://raw.githubusercontent.com/Tenount/XiNFC-WSC/main/install.sh)
```

### Manual Install

```bash
ssh root@192.168.1.1
wget https://github.com/Tenount/XiNFC-WSC/releases/latest/download/xinfc_*.apk
apk add ./xinfc_*.apk
```

### Post-install

1. Go to **System → NFC Configuration** in LuCI
2. Select your Wi-Fi interface or enter credentials manually
3. Click **Save & Apply**

## Usage

1. Open LuCI web interface
2. Navigate to **System → NFC Configuration**
3. Choose one of:
   - **Auto Configuration**: Select an existing Wi-Fi interface
   - **Manual Configuration**: Enter SSID, password, encryption type
4. Click **Save & Apply**

## Architecture

```text
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│  LuCI Web   │────▶│  xinfc-wsc   │────▶│  NFC Chip   │
│  Interface  │     │  (binary)    │     │  (I2C)      │
└─────────────┘     └──────────────┘     └─────────────┘
      JS                  C                  Hardware
```

## Security

- NFC broadcasts Wi-Fi credentials in plaintext
- Use Guest VLAN for NFC-connected devices
- NFC range is limited (~4-5 cm)

## NFC NDEF Types

The chip supports different NFC NDEF record types, depending on the
command used:

| Type | Command | Phone behavior |
|------|---------|---------------|
| **Wi-Fi (WSC)** | `xinfc-wsc <ssid> <password> <mode>` | Android shows Wi-Fi notification, one tap to connect. iOS opens App Store (no native Wi-Fi via NFC) |
| **URI** | `xinfc-wsc --write-url <url>` | Opens browser on tap. Works on most Android/iOS devices. WiFi-only tablets may skip |
| **Text** | `xinfc-wsc --write-text <message>` | Android shows notification with text. No native handler on iOS |
| **Empty** | `xinfc-wsc --clear` | Removes all data from chip. Phone shows "Empty tag" or nothing |

The NDEF format follows NFC Forum standards (RTD-URI for links,
RTD-Text for messages, WSC for Wi-Fi credentials). Third-party
tools like Flipper Zero may not display URI records, but NFC
Tools reads them correctly.

## CLI

Full command reference:

| Flag | Description |
|------|-------------|
| `--read` | Print current NDEF data (human-readable) |
| `--read --json` | Print current NDEF data as JSON |
| `--clear` | Erase all data from NFC chip |
| `--scan` | Probe I2C bus for NFC chip address |
| `--write-url <url>` | Write a URL link to the chip |
| `--write-text <message>` | Write a text note to the chip |
| `-h, --help` | Show usage |
| `-V, --version` | Show version and license |

## References

- [OpenWrt — Xiaomi AX3000T](https://openwrt.org/toh/xiaomi/ax3000t) — only device we've tested
- [Caian/xinfc](https://github.com/Caian/xinfc) — proof of concept for NDEF

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for dev setup, code style, and PR workflow.
