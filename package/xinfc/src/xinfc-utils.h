#pragma once

#include "wifi.h"

#include <stddef.h>
#include <stdio.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * Parse I2C address from hex string (e.g., "0x28").
 *
 * @param addr_str  Hex address string in format "0xMN" or "0XMN"
 * @return Parsed address (1-255), or negative on error:
 *         -1 = invalid format, -2 = invalid hex digit
 */
int parse_i2c_address(const char* addr_str);

/**
 * Look up encryption mode by name and return crypt/auth values.
 *
 * @param mode      Mode name (e.g., "psk2", "wep", "none")
 * @param out_crypt Output: encryption type
 * @param out_auth  Output: authentication type
 * @return 0 on success, -1 if mode not found or not supported
 */
int select_encryption_mode(const char* mode, enum wifi_crypt* out_crypt, enum wifi_auth* out_auth);

/**
 * Build WSC (Wi-Fi Simple Configuration) NDEF record.
 *
 * @param ssid      Wi-Fi SSID
 * @param password  Wi-Fi password
 * @param crypt     Encryption type
 * @param auth      Authentication type
 * @param buf       Output buffer for NDEF data
 * @param max_size  Size of output buffer
 * @return Number of bytes written, or 0 if buffer too small
 */
size_t make_wsc_ndef(const char* ssid, const char* password, enum wifi_crypt crypt,
                     enum wifi_auth auth, unsigned char* buf, size_t max_size);

/**
 * Build Text NDEF record (NFC Forum Well-known type 'T', UTF-8).
 *
 * @param text      UTF-8 text string (null-terminated)
 * @param buf       Output buffer for NDEF data
 * @param max_size  Size of output buffer
 * @return Number of bytes written, or 0 if text too long for buffer.
 *         Max text content: 150 bytes UTF-8 (10-byte NDEF overhead).
 */
size_t make_text_ndef(const char* text, unsigned char* buf, size_t max_size);

/**
 * Extract text content from a Text NDEF record.
 *
 * @param buf       Raw NDEF data (starting with 0x03 TLV)
 * @param size      Buffer size
 * @param out       Output buffer for extracted text (null-terminated)
 * @param out_size  Size of output buffer
 * @return Number of text bytes extracted (excluding null), or 0 on error
 */
size_t parse_text_ndef(const unsigned char* buf, unsigned int size, char* out, size_t out_size);

/**
 * Callback type for iterate_ndef_attrs().
 * Called for each WSC attribute, including those inside 0x100E Credential containers.
 */
typedef void (*wsc_attr_fn)(void* ctx, unsigned int attr_id, unsigned int attr_len,
                            const unsigned char* data);

/**
 * Walk NDEF TLV buffer and call cb for each WSC attribute.
 * Handles 0x100E Credential container recursion.
 *
 * @param buf   Raw NDEF data from NFC chip
 * @param size  Buffer size
 * @param cb    Callback for each WSC attribute
 * @param ctx   User context passed to cb
 * @return 0 on success, -1 if no valid NDEF found
 */
int iterate_ndef_attrs(const unsigned char* buf, unsigned int size, wsc_attr_fn cb, void* ctx);

/**
 * Serialize a WSC attribute to a JSON key-value fragment.
 *
 * @param dest      Output buffer
 * @param size      Size of output buffer
 * @param attr_id   WSC attribute ID
 * @param attr_len  Attribute payload length
 * @param data      Attribute payload
 * @return Bytes written, 0 if attribute should be skipped, or -1 on error
 */
int wsc_attr_to_json_kv(char* dest, size_t size, unsigned int attr_id, unsigned int attr_len,
                        const unsigned char* data);

/**
 * Parse NDEF buffer and print WSC attributes as JSON to stdout.
 *
 * @param ndef_rbuf   Raw NDEF data from NFC chip
 * @param size        Buffer size
 * @return 0 on success, -1 if no valid NDEF found
 */
int fprint_ndef_json(const unsigned char* ndef_rbuf, unsigned int size);

/* Core operations (xinfc-core.c) */
int apply_config(const char* ssid, const char* password, enum wifi_crypt crypt,
                 enum wifi_auth auth);
int read_ndef(int json_mode);
int clear_ndef(void);
int scan_i2c_bus(void);
int write_ndef_uri(const char* uri);
int write_ndef_text(const char* text);
int write_buffer(const unsigned char* buf, unsigned int aligned_size);
