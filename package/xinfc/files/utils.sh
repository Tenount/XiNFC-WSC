# shellcheck shell=sh
# shellcheck disable=SC2034,SC1091
. /lib/functions.sh

nfc_init() {
    config_load xinfc
    config_get NFC_IFACE global iface
    config_get_bool NFC_AUTO_SYNC global auto_sync 1
    config_get_bool NFC_LED_FEEDBACK global led_feedback 0
}

nfc_read_wifi_creds() {
    SSID=$(uci get wireless."$1".ssid 2>/dev/null)
    KEY=$(uci get wireless."$1".key 2>/dev/null)
    ENC=$(uci get wireless."$1".encryption 2>/dev/null)
}

nfc_led_feedback() {
    [ "$NFC_LED_FEEDBACK" = "1" ] || return 0
    if [ "$1" = "0" ]; then
        echo timer >/sys/class/leds/blue:status/trigger 2>/dev/null
        echo 50 >/sys/class/leds/blue:status/delay_on 2>/dev/null
        echo 50 >/sys/class/leds/blue:status/delay_off 2>/dev/null
        sleep 1
        echo default-on >/sys/class/leds/blue:status/trigger 2>/dev/null
    else
        echo none >/sys/class/leds/blue:status/trigger 2>/dev/null
        echo 0 >/sys/class/leds/blue:status/brightness 2>/dev/null
        echo timer >/sys/class/leds/yellow:status/trigger 2>/dev/null
        echo 50 >/sys/class/leds/yellow:status/delay_on 2>/dev/null
        echo 50 >/sys/class/leds/yellow:status/delay_off 2>/dev/null
        sleep 2
        echo none >/sys/class/leds/yellow:status/trigger 2>/dev/null
        echo 0 >/sys/class/leds/yellow:status/brightness 2>/dev/null
        echo default-on >/sys/class/leds/blue:status/trigger 2>/dev/null
    fi
    echo 1 >/sys/class/leds/blue:status/brightness 2>/dev/null
}
