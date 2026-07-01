/*
 * Unit tests for xinfc utility functions.
 *
 * Tests: parse_i2c_address, select_encryption_mode, make_wsc_ndef
 *
 * Build: gcc -Wall -Wextra -I../../package/xinfc/src test-wifi.c \
 *        ../../package/xinfc/src/xinfc-utils.c \
 *        ../../package/xinfc/src/wifi.c -lcmocka -o test-wifi
 */

#include "wifi.h"
#include "xinfc-utils.h"

#include <setjmp.h>
// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>
// clang-format on

/* ========================================================================== */
/*  parse_i2c_address tests                                                    */
/* ========================================================================== */

static void test_parse_i2c_address_valid_hex_lower(void** state) {
    (void)state;
    assert_int_equal(parse_i2c_address("0x28"), 0x28);
    assert_int_equal(parse_i2c_address("0x01"), 0x01);
    assert_int_equal(parse_i2c_address("0xff"), 0xff);
    assert_int_equal(parse_i2c_address("0x00"), 0x00);
}

static void test_parse_i2c_address_valid_hex_upper(void** state) {
    (void)state;
    assert_int_equal(parse_i2c_address("0X28"), 0x28);
    assert_int_equal(parse_i2c_address("0XFF"), 0xff);
}

static void test_parse_i2c_address_valid_mixed_case(void** state) {
    (void)state;
    assert_int_equal(parse_i2c_address("0xAa"), 0xaa);
    assert_int_equal(parse_i2c_address("0xFf"), 0xff);
}

static void test_parse_i2c_address_invalid_format(void** state) {
    (void)state;
    /* Too short */
    assert_int_equal(parse_i2c_address("0x1"), -1);
    /* Too long */
    assert_int_equal(parse_i2c_address("0x123"), -1);
    /* Missing 0x prefix */
    assert_int_equal(parse_i2c_address("28"), -1);
    /* Wrong prefix */
    assert_int_equal(parse_i2c_address("1x28"), -1);
    /* Empty */
    assert_int_equal(parse_i2c_address(""), -1);
}

static void test_parse_i2c_address_invalid_hex_digit(void** state) {
    (void)state;
    assert_int_equal(parse_i2c_address("0x2g"), -1);
    assert_int_equal(parse_i2c_address("0xzz"), -1);
}

/* ========================================================================== */
/*  select_encryption_mode tests                                               */
/* ========================================================================== */

static void test_select_encryption_mode_none(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("none", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_none);
    assert_int_equal(auth, wifi_auth_open);
}

static void test_select_encryption_mode_psk2_aes(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("psk2+aes", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa2_personal);
}

static void test_select_encryption_mode_psk2_tkip(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("psk2+tkip", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_tkip);
    assert_int_equal(auth, wifi_auth_wpa2_personal);
}

static void test_select_encryption_mode_psk_aes(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("psk+aes", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa_personal);
}

static void test_select_encryption_mode_wep(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wep", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_wep);
    assert_int_equal(auth, wifi_auth_open);
}

static void test_select_encryption_mode_wep_shared(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wep+shared", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_wep);
    assert_int_equal(auth, wifi_auth_shared);
}

static void test_select_encryption_mode_wpa2_aes(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wpa2+aes", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa2_enterprise);
}

static void test_select_encryption_mode_wpa_aes(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wpa+aes", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa_enterprise);
}

static void test_select_encryption_mode_owe(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("owe", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_none);
    assert_int_equal(auth, wifi_auth_open);
}

static void test_select_encryption_mode_wep_open(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wep+open", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_wep);
    assert_int_equal(auth, wifi_auth_open);
}

static void test_select_encryption_mode_psk2_aes_openwrt(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("psk2+aes", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa2_personal);
}

static void test_select_encryption_mode_psk2_tkip_aes_openwrt(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("psk2+tkip+aes", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_tkip_aes);
    assert_int_equal(auth, wifi_auth_wpa2_personal);
}

static void test_select_encryption_mode_psk_mixed(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("psk-mixed", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa2_personal);
}

static void test_select_encryption_mode_sae(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("sae", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa2_personal);
}

static void test_select_encryption_mode_wpa2_tkip_ccmp(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wpa2+tkip+ccmp", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_tkip_aes);
    assert_int_equal(auth, wifi_auth_wpa2_enterprise);
}

static void test_select_encryption_mode_wpa_mixed(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wpa-mixed", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa2_enterprise);
}

static void test_select_encryption_mode_wpa3(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wpa3", &crypt, &auth), 0);
    assert_int_equal(crypt, wifi_crypt_aes);
    assert_int_equal(auth, wifi_auth_wpa2_enterprise);
}

static void test_select_encryption_mode_unknown(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("foobar", &crypt, &auth), -1);
}

static void test_select_encryption_mode_wpa3_192bit(void** state) {
    (void)state;
    enum wifi_crypt crypt;
    enum wifi_auth auth;
    assert_int_equal(select_encryption_mode("wpa3-192bit", &crypt, &auth), -1);
}

/* ========================================================================== */
/*  make_wsc_ndef tests                                                        */
/* ========================================================================== */

static void test_make_wsc_ndef_basic(void** state) {
    (void)state;
    unsigned char buf[256];
    size_t size = make_wsc_ndef("MySSID", "password123", wifi_crypt_aes, wifi_auth_wpa2_personal,
                                buf, sizeof(buf));
    assert_true(size > 0 && size <= sizeof(buf));

    /* NDEF header: type length, payload type, MB/ME flags */
    assert_int_equal(buf[0], 0x03); /* NDEF record: MB=1, ME=1, CF=0, SR=1, TNF=0x03 */

    /* Application type: "application/vnd.wfa.wsc" (24 bytes at offset 5) */
    const char expected_app[] = "application/vnd.wfa.wsc";
    assert_memory_equal(&buf[5], expected_app, sizeof(expected_app) - 1);
}

static void test_make_wsc_ndef_empty_ssid(void** state) {
    (void)state;
    unsigned char buf[256];
    /* SSID must be at least SSID_MIN (1) chars per validation, but make_wsc_ndef
     * doesn't validate - it just builds the NDEF. Empty SSID should work. */
    size_t size = make_wsc_ndef("", "password", wifi_crypt_none, wifi_auth_open, buf, sizeof(buf));
    assert_true(size > 1 && size <= sizeof(buf));
}

static void test_make_wsc_ndef_buffer_too_small(void** state) {
    (void)state;
    unsigned char buf[10];
    size_t size = make_wsc_ndef("MySSID", "password123", wifi_crypt_aes, wifi_auth_wpa2_personal,
                                buf, sizeof(buf));
    assert_int_equal(size, 0);
}

static void test_make_wsc_ndef_wep_encryption(void** state) {
    (void)state;
    unsigned char buf[256];
    size_t size =
        make_wsc_ndef("TestSSID", "testpass", wifi_crypt_wep, wifi_auth_open, buf, sizeof(buf));
    assert_true(size > 1 && size <= sizeof(buf));

    /* Verify NDEF header is present */
    assert_int_equal(buf[0], 0x03);
}

static void test_make_wsc_ndef_max_sized_ssid(void** state) {
    (void)state;
    unsigned char buf[256];
    /* SSID_MAX = 32 */
    char ssid[33];
    memset(ssid, 'A', 32);
    ssid[32] = '\0';

    size_t size =
        make_wsc_ndef(ssid, "password", wifi_crypt_aes, wifi_auth_wpa2_personal, buf, sizeof(buf));
    assert_true(size > 1 && size <= sizeof(buf));
    assert_int_equal(buf[0], 0x03);
}

/* ========================================================================== */
/*  make_text_ndef tests                                                       */
/* ========================================================================== */

static void test_make_text_ndef_ascii(void** state) {
    (void)state;
    unsigned char buf[256];
    /* "Hello" = 5 bytes, total = 10 + 5 = 15 */
    size_t size = make_text_ndef("Hello", buf, sizeof(buf));
    assert_int_equal(size, 15);
    assert_int_equal(buf[0], 0x03);
    assert_int_equal(buf[5], 'T');
    assert_memory_equal(&buf[9], "Hello", 5);
    assert_int_equal(buf[14], 0xFE);
}

static void test_make_text_ndef_cyrillic(void** state) {
    (void)state;
    unsigned char buf[256];
    /* "Привет" = 12 bytes UTF-8, total = 10 + 12 = 22 */
    const char text[] = "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82";
    size_t size = make_text_ndef(text, buf, sizeof(buf));
    assert_int_equal(size, 22);
    assert_int_equal(buf[0], 0x03);
    assert_int_equal(buf[5], 'T');
    assert_memory_equal(&buf[9], text, 12);
    assert_int_equal(buf[21], 0xFE);
}

static void test_make_text_ndef_emoji(void** state) {
    (void)state;
    unsigned char buf[256];
    /* "😀" U+1F600 = F0 9F 98 80, 4 bytes, total = 10 + 4 = 14 */
    const char text[] = "\xf0\x9f\x98\x80";
    size_t size = make_text_ndef(text, buf, sizeof(buf));
    assert_int_equal(size, 14);
    assert_int_equal(buf[0], 0x03);
    assert_int_equal(buf[5], 'T');
    assert_int_equal(buf[9], 0xF0);
    assert_int_equal(buf[10], 0x9F);
    assert_int_equal(buf[11], 0x98);
    assert_int_equal(buf[12], 0x80);
    assert_int_equal(buf[13], 0xFE);
}

static void test_make_text_ndef_mixed(void** state) {
    (void)state;
    unsigned char buf[256];
    /* "Hi \xf0\x9f\x98\x80 \xd0\x9f\xd1\x80" = 2+1+4+1+4 = 12 bytes */
    const char text[] = "Hi \xf0\x9f\x98\x80 \xd0\x9f\xd1\x80";
    size_t size = make_text_ndef(text, buf, sizeof(buf));
    assert_int_equal(size, 10 + 12);
    assert_int_equal(buf[5], 'T');
}

static void test_make_text_ndef_boundary_150_bytes(void** state) {
    (void)state;
    unsigned char buf[256];
    char text[151];
    memset(text, 'A', 150);
    text[150] = '\0';
    /* total = 10 + 150 = 160 = MAX_NDEF_BUF_SIZE, fits exactly */
    size_t size = make_text_ndef(text, buf, 160);
    assert_int_equal(size, 160);
    assert_int_equal(buf[0], 0x03);
    assert_int_equal(buf[159], 0xFE);
}

static void test_make_text_ndef_over_limit_151_bytes(void** state) {
    (void)state;
    unsigned char buf[256];
    char text[152];
    memset(text, 'A', 151);
    text[151] = '\0';
    /* total = 10 + 151 = 161 > 160, reject */
    size_t size = make_text_ndef(text, buf, 160);
    assert_int_equal(size, 0);
}

static void test_make_text_ndef_empty(void** state) {
    (void)state;
    unsigned char buf[256];
    /* total = 10 + 0 = 10 */
    size_t size = make_text_ndef("", buf, sizeof(buf));
    assert_int_equal(size, 10);
    assert_int_equal(buf[0], 0x03);
    assert_int_equal(buf[9], 0xFE);
}

static void test_make_text_ndef_emoji_boundary(void** state) {
    (void)state;
    unsigned char buf[256];
    /* 37 emojis × 4 bytes = 148 → total = 158, fits in 160 */
    char text[149];
    int j = 0;
    for (int k = 0; k < 37; k++) {
        text[j++] = (char)0xF0;
        text[j++] = (char)0x9F;
        text[j++] = (char)0x98;
        text[j++] = (char)0x80;
    }
    text[j] = '\0';
    size_t size = make_text_ndef(text, buf, 160);
    assert_int_equal(size, 158);

    /* 38 emojis × 4 bytes = 152 → total = 162 > 160, reject */
    char text2[157];
    j = 0;
    for (int k = 0; k < 38; k++) {
        text2[j++] = (char)0xF0;
        text2[j++] = (char)0x9F;
        text2[j++] = (char)0x98;
        text2[j++] = (char)0x80;
    }
    text2[j] = '\0';
    size = make_text_ndef(text2, buf, 160);
    assert_int_equal(size, 0);
}

/* ========================================================================== */
/*  parse_text_ndef round-trip tests                                           */
/* ========================================================================== */

static void test_parse_text_ndef_emoji_roundtrip(void** state) {
    (void)state;
    unsigned char buf[256];
    const char emoji[] = "\xf0\x9f\x98\x80";
    size_t size = make_text_ndef(emoji, buf, sizeof(buf));
    assert_true(size > 0);

    char out[256];
    size_t text_len = parse_text_ndef(buf, (unsigned int)size, out, sizeof(out));
    assert_int_equal(text_len, 4);
    assert_memory_equal(out, emoji, 4);
    assert_int_equal(out[4], '\0');
}

static void test_parse_text_ndef_cyrillic_roundtrip(void** state) {
    (void)state;
    unsigned char buf[256];
    const char cyr[] = "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82";
    size_t size = make_text_ndef(cyr, buf, sizeof(buf));
    assert_true(size > 0);

    char out[256];
    size_t text_len = parse_text_ndef(buf, (unsigned int)size, out, sizeof(out));
    assert_int_equal(text_len, 12);
    assert_memory_equal(out, cyr, 12);
}

static void test_parse_text_ndef_mixed_roundtrip(void** state) {
    (void)state;
    unsigned char buf[256];
    const char mixed[] = "Hi \xf0\x9f\x98\x80 \xd0\x9f\xd1\x80";
    size_t size = make_text_ndef(mixed, buf, sizeof(buf));
    assert_true(size > 0);

    char out[256];
    size_t text_len = parse_text_ndef(buf, (unsigned int)size, out, sizeof(out));
    assert_int_equal(text_len, strlen(mixed));
    assert_string_equal(out, mixed);
}

static void test_parse_text_ndef_invalid_buffer(void** state) {
    (void)state;
    unsigned char buf[4] = {0x00, 0x01, 0x02, 0x03};
    char out[64];
    size_t text_len = parse_text_ndef(buf, 4, out, sizeof(out));
    assert_int_equal(text_len, 0);
}

/* ========================================================================== */
/* wsc_attr_to_json_kv tests                                                 */
/* ========================================================================== */

static void test_wsc_attr_to_json_kv_string_escape(void** state) {
    (void)state;
    char buf[256];
    const unsigned char ssid[] = "My\"Net\\work";
    int len = wsc_attr_to_json_kv(buf, sizeof(buf), 0x1045, sizeof(ssid) - 1, ssid);
    assert_true(len > 0);
    assert_string_equal(buf, "\"ssid\": \"My\\\"Net\\\\work\"");
}

static void test_wsc_attr_to_json_kv_int_conversion(void** state) {
    (void)state;
    char buf[256];
    const unsigned char auth[] = {0x00, 0x22};
    int len = wsc_attr_to_json_kv(buf, sizeof(buf), 0x1003, 2, auth);
    assert_true(len > 0);
    assert_string_equal(buf, "\"auth_type\": 34");
}

static void test_wsc_attr_to_json_kv_untracked_attr(void** state) {
    (void)state;
    char buf[256];
    const unsigned char dummy[] = {0xFF};
    int len = wsc_attr_to_json_kv(buf, sizeof(buf), 0x1026, 1, dummy);
    assert_int_equal(len, 0);
}

static void test_make_wsc_ndef_max_sized_password(void** state) {
    (void)state;
    unsigned char buf[256];
    /* PASS_MAX = 63 */
    char pass[64];
    memset(pass, 'B', 63);
    pass[63] = '\0';

    size_t size =
        make_wsc_ndef("SSID", pass, wifi_crypt_aes, wifi_auth_wpa2_personal, buf, sizeof(buf));
    assert_true(size > 1 && size <= sizeof(buf));
    assert_int_equal(buf[0], 0x03);
}

/* ========================================================================== */
/*  Main                                                                       */
/* ========================================================================== */

int main(void) {
    const struct CMUnitTest tests[] = {
        /* parse_i2c_address */
        cmocka_unit_test(test_parse_i2c_address_valid_hex_lower),
        cmocka_unit_test(test_parse_i2c_address_valid_hex_upper),
        cmocka_unit_test(test_parse_i2c_address_valid_mixed_case),
        cmocka_unit_test(test_parse_i2c_address_invalid_format),
        cmocka_unit_test(test_parse_i2c_address_invalid_hex_digit),

        /* select_encryption_mode */
        cmocka_unit_test(test_select_encryption_mode_none),
        cmocka_unit_test(test_select_encryption_mode_owe),
        cmocka_unit_test(test_select_encryption_mode_psk2_aes),
        cmocka_unit_test(test_select_encryption_mode_psk2_tkip),
        cmocka_unit_test(test_select_encryption_mode_psk_aes),
        cmocka_unit_test(test_select_encryption_mode_wep),
        cmocka_unit_test(test_select_encryption_mode_wep_shared),
        cmocka_unit_test(test_select_encryption_mode_wep_open),
        cmocka_unit_test(test_select_encryption_mode_wpa2_aes),
        cmocka_unit_test(test_select_encryption_mode_wpa_aes),
        cmocka_unit_test(test_select_encryption_mode_psk2_aes_openwrt),
        cmocka_unit_test(test_select_encryption_mode_psk2_tkip_aes_openwrt),
        cmocka_unit_test(test_select_encryption_mode_psk_mixed),
        cmocka_unit_test(test_select_encryption_mode_sae),
        cmocka_unit_test(test_select_encryption_mode_wpa2_tkip_ccmp),
        cmocka_unit_test(test_select_encryption_mode_wpa_mixed),
        cmocka_unit_test(test_select_encryption_mode_wpa3),
        cmocka_unit_test(test_select_encryption_mode_unknown),
        cmocka_unit_test(test_select_encryption_mode_wpa3_192bit),

        /* make_wsc_ndef */
        cmocka_unit_test(test_make_wsc_ndef_basic),
        cmocka_unit_test(test_make_wsc_ndef_empty_ssid),
        cmocka_unit_test(test_make_wsc_ndef_buffer_too_small),
        cmocka_unit_test(test_make_wsc_ndef_wep_encryption),
        cmocka_unit_test(test_make_wsc_ndef_max_sized_ssid),
        cmocka_unit_test(test_make_wsc_ndef_max_sized_password),

        /* make_text_ndef */
        cmocka_unit_test(test_make_text_ndef_ascii),
        cmocka_unit_test(test_make_text_ndef_cyrillic),
        cmocka_unit_test(test_make_text_ndef_emoji),
        cmocka_unit_test(test_make_text_ndef_mixed),
        cmocka_unit_test(test_make_text_ndef_boundary_150_bytes),
        cmocka_unit_test(test_make_text_ndef_over_limit_151_bytes),
        cmocka_unit_test(test_make_text_ndef_empty),
        cmocka_unit_test(test_make_text_ndef_emoji_boundary),

        /* parse_text_ndef round-trip */
        cmocka_unit_test(test_parse_text_ndef_emoji_roundtrip),
        cmocka_unit_test(test_parse_text_ndef_cyrillic_roundtrip),
        cmocka_unit_test(test_parse_text_ndef_mixed_roundtrip),
        cmocka_unit_test(test_parse_text_ndef_invalid_buffer),

        /* wsc_attr_to_json_kv */
        cmocka_unit_test(test_wsc_attr_to_json_kv_string_escape),
        cmocka_unit_test(test_wsc_attr_to_json_kv_int_conversion),
        cmocka_unit_test(test_wsc_attr_to_json_kv_untracked_attr),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
