#!/bin/sh
# shellcheck disable=SC1091
. /lib/functions.sh
. /usr/share/xinfc/utils.sh

TAG="xinfc-nfc-sync"
LOCKFILE="/var/run/xinfc-sync.lock"

if [ -f "$LOCKFILE" ]; then
    read -r LOCKPID <"$LOCKFILE"
    if [ -n "$LOCKPID" ] && kill -0 "$LOCKPID" 2>/dev/null; then
        logger -t "$TAG" "Already running (pid $LOCKPID), skipping"
        exit 0
    fi
    rm -f "$LOCKFILE"
fi

echo "$$" >"$LOCKFILE"
trap 'rm -f $LOCKFILE' EXIT

nfc_init
[ "$NFC_AUTO_SYNC" = "1" ] && [ -n "$NFC_IFACE" ] || exit 0
sleep 2

nfc_read_wifi_creds "$NFC_IFACE"
[ -n "$SSID" ] || exit 0

logger -t "$TAG" -p info "Auto-sync NFC: ssid=$SSID iface=$NFC_IFACE enc=$ENC"
output=$(/usr/sbin/xinfc-wsc "$SSID" "$KEY" "$ENC" 2>&1)
rc=$?
echo "$output" | logger -t "$TAG" -p daemon.info

nfc_led_feedback "$rc"
logger -t "$TAG" -p info "Auto-sync complete"
