#!/bin/sh
# shellcheck disable=SC1091
. /usr/share/xinfc/utils.sh

nfc_init

output=$(/usr/sbin/xinfc-wsc --clear 2>&1)
rc=$?

[ -z "$output" ] || echo "$output" >&2

nfc_led_feedback "$rc"
exit "$rc"
