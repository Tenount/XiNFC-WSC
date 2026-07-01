#!/bin/sh
# shellcheck disable=SC1091
. /usr/share/xinfc/utils.sh

nfc_init

[ $# -eq 3 ] || exit 10

SSID="$1"
PASS="$2"
ENC="$3"

output=$(/usr/sbin/xinfc-wsc "$SSID" "$PASS" "$ENC" 2>&1)
rc=$?

[ -z "$output" ] || echo "$output" >&2

nfc_led_feedback "$rc"
exit "$rc"
