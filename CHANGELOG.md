# Changelog

## [2.0.0] - 2026-07-01

First public release.

### Added

- Write Wi-Fi credentials (WSC NDEF) to NFC chip via I2C
- Write URL links, text messages, or clear the chip
- LuCI web interface with Russian localization
- Auto-detect and select any Wi-Fi interface (main or guest)
- Auto-sync on Wi-Fi password change via hotplug
- Support for WPA2, WPA3, WPA2/3 mixed, Enterprise, and open networks
- Diagnostics: read NFC tag, I2C bus scan, version info
- One-click admin URL write to NFC
- Packages for OpenWrt 24.10 (ipk) and 25.12 (apk)
- clang-tidy static analysis integrated into MegaLinter; all warnings fail CI
- OpenWrt-style encryption mode names (psk2+aes, sae, wpa2+tkip, etc.)
- Unit test for flock-based exclusive lock
- `PKG_LICENSE` and `URL` metadata in OpenWrt Makefiles
- Reusable `make_text_ndef()` / `parse_text_ndef()` NDEF helpers
- LuCI text counter with char/byte counts and 150-byte validation
- Unit tests for text NDEF (ASCII, Cyrillic, emoji, boundary, round-trip)
