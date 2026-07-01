#!/bin/sh
# shellcheck disable=SC1091
. /lib/functions.sh
. /usr/share/xinfc/utils.sh

TAG="xinfc-nfc-sync"

nfc_init
[ "$NFC_AUTO_SYNC" = "1" ] || exit 0
[ -n "$NFC_IFACE" ] || exit 0
[ "$ACTION" = "ifup" ] || exit 0
[ "$INTERFACE" = "$NFC_IFACE" ] || exit 0

sleep 3

nfc_read_wifi_creds "$NFC_IFACE"
[ -n "$SSID" ] || exit 0

logger -t "$TAG" -p info "Syncing Wi-Fi to NFC on $INTERFACE: ssid=$SSID"
output=$(/usr/sbin/xinfc-wsc "$SSID" "$KEY" "$ENC" 2>&1)
rc=$?
echo "$output" | logger -t "$TAG" -p daemon.info

nfc_led_feedback "$rc"
logger -t "$TAG" -p info "NFC sync complete"
