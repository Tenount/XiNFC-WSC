#!/bin/sh
ip="$(uci get network.lan.ipaddr 2>/dev/null)" || exit 1
[ -n "$ip" ] || exit 1
exec /usr/sbin/xinfc-wsc --write-url "http://$ip/cgi-bin/luci/admin/system/xinfc"
