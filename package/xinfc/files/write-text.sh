#!/bin/sh
# shellcheck disable=SC1091
. /usr/share/xinfc/utils.sh

nfc_init

[ $# -ge 1 ] || exit 10

output=$(/usr/sbin/xinfc-wsc --write-text "$1" 2>&1)
rc=$?

[ -z "$output" ] || echo "$output" >&2

nfc_led_feedback "$rc"
exit "$rc"
