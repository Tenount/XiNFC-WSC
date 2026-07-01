/*
 * WSC (Wi-Fi Simple Configuration v2.0.5 §8.3) constants.
 *
 *   Authentication Type (0x1003):
 *     0x0001 = Open
 *     0x0002 = WPA PSK (WPA1 Personal)
 *     0x0004 = Shared Key (WEP)
 *     0x0008 = WPA Enterprise (WPA1 Ent)
 *     0x0010 = WPA2 Enterprise
 *     0x0020 = WPA2 PSK
 *
 *   Encryption Type (0x100F):
 *     0x0001 = None
 *     0x0002 = WEP
 *     0x0004 = TKIP
 *     0x0008 = AES (CCMP)
 *
 */

#pragma once

/* Source: https://w1.fi/cgit/hostap/tree/src/wps/wps_defs.h */

enum wifi_crypt {
    wifi_crypt_none = 0x0001,    /* WPS_ENCR_NONE */
    wifi_crypt_wep = 0x0002,     /* WPS_ENCR_WEP */
    wifi_crypt_tkip = 0x0004,    /* WPS_ENCR_TKIP */
    wifi_crypt_aes = 0x0008,     /* WPS_ENCR_AES */
    wifi_crypt_tkip_aes = 0x000C /* WPS_ENCR_TKIP | WPS_ENCR_AES = 0x0004 | 0x0008 */
};

enum wifi_auth {
    wifi_auth_open = 0x0001,           /* WPS_AUTH_OPEN */
    wifi_auth_shared = 0x0004,         /* WPS_AUTH_SHARED */
    wifi_auth_wpa_personal = 0x0002,   /* WPS_AUTH_WPAPSK */
    wifi_auth_wpa2_personal = 0x0022,  /* WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK */
    wifi_auth_wpa_enterprise = 0x0008, /* WPS_AUTH_WPA */
    wifi_auth_wpa2_enterprise = 0x0010 /* WPS_AUTH_WPA2 */
};

#define SSID_MIN 1
#define SSID_MAX 32
#define PASS_MIN 8
#define PASS_MAX 63

struct wifi_modes {
    const char* name;
    const char* message;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
};

extern struct wifi_modes g_openwrt_modes[];
