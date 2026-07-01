#include "wifi.h"

#include <stddef.h>

struct wifi_modes g_openwrt_modes[] = {
    /*
     * Open / No encryption
     */
    {.name = "none", .crypt = wifi_crypt_none, .auth = wifi_auth_open},
    {.name = "owe", .crypt = wifi_crypt_none, .auth = wifi_auth_open},

    /*
     * WEP
     */
    {.name = "wep", .crypt = wifi_crypt_wep, .auth = wifi_auth_open},
    {.name = "wep+open", .crypt = wifi_crypt_wep, .auth = wifi_auth_open},
    {.name = "wep+shared", .crypt = wifi_crypt_wep, .auth = wifi_auth_shared},

    /*
     * WPA1 Personal (PSK)
     */
    {.name = "psk", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa_personal},
    {.name = "psk+aes", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa_personal},
    {.name = "psk+ccmp", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa_personal},
    {.name = "psk+tkip", .crypt = wifi_crypt_tkip, .auth = wifi_auth_wpa_personal},
    {.name = "psk+tkip+aes", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa_personal},
    {.name = "psk+tkip+ccmp", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa_personal},

    /*
     * WPA2 Personal (PSK) / WPA1/WPA2 Mixed
     */
    {.name = "psk2", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk2+aes", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk2+ccmp", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk2+tkip", .crypt = wifi_crypt_tkip, .auth = wifi_auth_wpa2_personal},
    {.name = "psk2+tkip+aes", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk2+tkip+ccmp", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk-mixed", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk-mixed+aes", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk-mixed+ccmp", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk-mixed+tkip", .crypt = wifi_crypt_tkip, .auth = wifi_auth_wpa2_personal},
    {.name = "psk-mixed+tkip+aes", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa2_personal},
    {.name = "psk-mixed+tkip+ccmp", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa2_personal},

    /*
     * WPA2/WPA3 Mixed (SAE + PSK)
     * WSC has no SAE auth type bit, so this is announced as WPA2 only.
     */
    {.name = "sae-mixed",
     .crypt = wifi_crypt_aes,
     .auth = wifi_auth_wpa2_personal,
     .message = "Mixed WPA2/WPA3 will be announced as WPA2!"},
    {.name = "sae",
     .crypt = wifi_crypt_aes,
     .auth = wifi_auth_wpa2_personal,
     .message = "WPA3 has no WSC bits — announced as WPA2!"},

    /*
     * WPA1 Enterprise
     */
    {.name = "wpa", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa_enterprise},
    {.name = "wpa+aes", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa_enterprise},
    {.name = "wpa+ccmp", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa_enterprise},
    {.name = "wpa+tkip", .crypt = wifi_crypt_tkip, .auth = wifi_auth_wpa_enterprise},
    {.name = "wpa+tkip+aes", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa_enterprise},
    {.name = "wpa+tkip+ccmp", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa_enterprise},

    /*
     * WPA2 Enterprise / WPA1/WPA2 Mixed Enterprise
     */
    {.name = "wpa2", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa2+aes", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa2+ccmp", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa2+tkip", .crypt = wifi_crypt_tkip, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa2+tkip+aes", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa2+tkip+ccmp", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa-mixed", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa-mixed+aes", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa-mixed+ccmp", .crypt = wifi_crypt_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa-mixed+tkip", .crypt = wifi_crypt_tkip, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa-mixed+tkip+aes", .crypt = wifi_crypt_tkip_aes, .auth = wifi_auth_wpa2_enterprise},
    {.name = "wpa-mixed+tkip+ccmp",
     .crypt = wifi_crypt_tkip_aes,
     .auth = wifi_auth_wpa2_enterprise},

    /*
     * WPA2/WPA3 Enterprise Mixed
     * WSC has no WPA3 bits, so this is announced as WPA2.
     */
    {.name = "wpa3-mixed",
     .crypt = wifi_crypt_aes,
     .auth = wifi_auth_wpa2_enterprise,
     .message = "Mixed WPA2/WPA3 Enterprise will be announced as WPA2!"},
    {.name = "wpa3",
     .crypt = wifi_crypt_aes,
     .auth = wifi_auth_wpa2_enterprise,
     .message = "WPA3 has no WSC bits — announced as WPA2!"},

    /* sentinel-terminated array */
    {0}};
