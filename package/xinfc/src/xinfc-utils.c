#include "xinfc-utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_i2c_address(const char* addr_str) {
    char* endptr;
    errno = 0;
    const long addr = strtol(addr_str, &endptr, 16);

    if (endptr == addr_str || *endptr != '\0' || errno == ERANGE) {
        fprintf(stderr, "xinfc: invalid I2C device\n");
        return -1;
    }

    /* Validate format: must be 0x + exactly 2 hex digits */
    const ptrdiff_t len = endptr - addr_str;
    if (len != 4) {
        fprintf(stderr, "xinfc: invalid I2C device\n");
        return -1;
    }

    return (int)addr;
}

int select_encryption_mode(const char* mode, enum wifi_crypt* out_crypt, enum wifi_auth* out_auth) {
    for (struct wifi_modes* m = g_openwrt_modes; m->name; ++m) {
        if (strcmp(mode, m->name) == 0) {
            if (m->message) {
                fprintf(stderr, "xinfc: %s: %s\n", "warning", m->message);
            }
            *out_crypt = m->crypt;
            *out_auth = m->auth;
            return 0;
        }
    }

    fprintf(stderr, "xinfc: unknown encryption mode\n");
    return -1;
}

/*
 * Build a WSC (Wi-Fi Simple Configuration) NDEF message for NFC Type 2 Tag.
 *
 * Layout (NFC Type 2 Tag → NDEF → WSC):
 *
 *   NFC Type 2 Tag TLV (ISO/IEC 14443-3):
 *     [0x03]       TLV type: NDEF message (Type 2 Tag spec §2.3)
 *     [length]     TLV value length
 *       NDEF record (NFC Forum NDEF spec §2.3):
 *         [0xD2]   NDEF header: MB=1, ME=1, SR=1, TNF=2 (Well-known type)
 *         [0x17]   Type name length = 23 ("application/vnd.wfa.wsc")
 *         [length] Payload length (short record, 1 byte)
 *         "application/vnd.wfa.wsc"  NDEF type (NFC Forum well-known type for WSC)
 *           WSC payload (Wi-Fi Simple Configuration v2.0.5, §8.3 — Credential):
 *             [0x100E]  WSC attribute: Credential container
 *               [0x1026]  WSC attribute: Network Index (always 1)
 *               [0x1045]  WSC attribute: SSID
 *               [0x1003]  WSC attribute: Authentication Type
 *               [0x100F]  WSC attribute: Encryption Type
 *               [0x1027]  WSC attribute: Network Key (password)
 *               [0x1020]  WSC attribute: MAC Address
 *   [0xFE]       TLV terminator (Type 2 Tag spec §2.3)
 */
size_t make_wsc_ndef(const char* ssid, const char* password, enum wifi_crypt crypt,
                     enum wifi_auth auth, unsigned char* buf, size_t max_size) {
    const char ndef_app[] = "application/vnd.wfa.wsc";
    const size_t ssid_len = strlen(ssid);
    /* For open networks, omit the password (WSC spec §8.3) */
    /* nosemgrep: raptor-typos */
    const size_t pass_len = (crypt == wifi_crypt_none) ? 0 : strlen(password);

    if (69 + ssid_len + pass_len > max_size) {
        return 0;
    }

    /* WSC payload = 35 bytes of fixed overhead + ssid + password */
    const size_t payload_len = 35 + ssid_len + pass_len;
    size_t i = 0;

    /*
     * NFC Type 2 Tag TLV block (ISO/IEC 14443-3 / NFC Type 2 Tag spec §2.3)
     * TLV type 0x03 = NDEF message
     * TLV length = 30 (NDEF header+type) + payload_len
     */
    buf[i++] = 0x03;                              /* TLV type: NDEF message */
    buf[i++] = (unsigned char)(30 + payload_len); /* TLV value length */

    /*
     * NDEF record header (NFC Forum NDEF 1.0 §2.3)
     * 0xD2 = MB=1 (Message Begin), ME=1 (Message End),
     *        SR=1 (Short Record — payload len in 1 byte),
     *        TNF=010b (Well-known type)
     */
    buf[i++] = 0xd2;                             /* NDEF header flags */
    buf[i++] = 0x17;                             /* Type name length = 23 bytes */
    buf[i++] = (unsigned char)(4 + payload_len); /* NDEF payload length (SR=1, 1 byte) */

    /*
     * NDEF type: NFC Forum well-known type for WSC
     * (NFC Forum WSC Application v1.0 / "application/vnd.wfa.wsc")
     */
    for (size_t j = 0; j < sizeof(ndef_app) - 1; j++) {
        buf[i++] = (unsigned char)ndef_app[j];
    }

    /*
     * WSC Credential container (Wi-Fi Simple Configuration v2.0.5 §8.3)
     * Attribute 0x100E — groups all credential fields together.
     * Length = payload_len.
     */
    buf[i++] = 0x10; /* attr_id = 0x100E (Credential) */
    buf[i++] = 0x0e;
    buf[i++] = (unsigned char)((payload_len >> 8) & 0xFF); /* attr length (big-endian) */
    buf[i++] = (unsigned char)(payload_len & 0xFF);

    /*
     * Network Index (WSC §8.3, attr 0x1026, length 1)
     * Always 1 — we only write one credential.
     */
    buf[i++] = 0x10; /* attr_id = 0x1026 (Network Index) */
    buf[i++] = 0x26;
    buf[i++] = 0x00; /* attr length = 1 */
    buf[i++] = 0x01;
    buf[i++] = 0x01; /* value = 1 (first credential) */

    /*
     * SSID (WSC §8.3, attr 0x1045)
     * The Wi-Fi network name, variable length (1-32 bytes).
     */
    buf[i++] = 0x10; /* attr_id = 0x1045 (SSID) */
    buf[i++] = 0x45;
    buf[i++] = (unsigned char)((ssid_len >> 8) & 0xFF); /* attr length (big-endian) */
    buf[i++] = (unsigned char)(ssid_len & 0xFF);

    for (size_t j = 0; j < ssid_len; ++j) {
        buf[i++] = (unsigned char)ssid[j];
    }

    /*
     * Authentication Type (WSC §8.3, attr 0x1003, length 2)
     * Bitmask: 0x0001=Open, 0x0002=WPA PSK, 0x0008=WPA Ent, 0x0010=WPA2, 0x0020=WPA2 PSK
     */
    buf[i++] = 0x10; /* attr_id = 0x1003 (Authentication Type) */
    buf[i++] = 0x03;
    buf[i++] = 0x00; /* attr length = 2 */
    buf[i++] = 0x02;
    buf[i++] = (unsigned char)((auth >> 8) & 0xFF); /* value (big-endian 16-bit) */
    buf[i++] = (unsigned char)(auth & 0xFF);

    /*
     * Encryption Type (WSC §8.3, attr 0x100F, length 2)
     * Bitmask: 0x0001=None, 0x0002=WEP, 0x0004=TKIP, 0x0008=AES
     */
    buf[i++] = 0x10; /* attr_id = 0x100F (Encryption Type) */
    buf[i++] = 0x0f;
    buf[i++] = 0x00; /* attr length = 2 */
    buf[i++] = 0x02;
    buf[i++] = (unsigned char)((crypt >> 8) & 0xFF); /* value (big-endian 16-bit) */
    buf[i++] = (unsigned char)(crypt & 0xFF);

    /*
     * Network Key (WSC §8.3, attr 0x1027)
     * The Wi-Fi password/passphrase, variable length (8-63 bytes).
     */
    buf[i++] = 0x10; /* attr_id = 0x1027 (Network Key) */
    buf[i++] = 0x27;
    buf[i++] = (unsigned char)((pass_len >> 8) & 0xFF); /* attr length (big-endian) */
    buf[i++] = (unsigned char)(pass_len & 0xFF);

    for (size_t j = 0; j < pass_len; ++j) {
        buf[i++] = (unsigned char)password[j];
    }

    /*
     * MAC Address (WSC §8.3, attr 0x1020, length 6)
     * Placeholder — we don't know the AP MAC at write time.
     * Value: FF:FF:FF:FF:FF:FF. This is accepted by Android/iOS NFC readers.
     */
    buf[i++] = 0x10; /* attr_id = 0x1020 (MAC Address) */
    buf[i++] = 0x20;
    buf[i++] = 0x00; /* attr length = 6 */
    buf[i++] = 0x06;
    buf[i++] = 0xFF; /* placeholder MAC */
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;

    /*
     * TLV terminator (NFC Type 2 Tag spec §2.3)
     * Type 0xFE marks the end of TLV blocks. No length/value follows.
     */
    buf[i++] = 0xFE;

    return i;
}

/*
 * Build a Text NDEF message for NFC Type 2 Tag.
 *
 * Layout:
 *   [0x03] [tlv_len]                    TLV header (NDEF message)
 *     [0xD1] [0x01] [payload_len] ['T'] NDEF record (MB=1,ME=1,SR=1,TNF=1)
 *       [status] ['e'] ['n'] [text...]  Payload: UTF-8, language "en"
 *   [0xFE]                              TLV terminator
 *
 * Overhead = 10 bytes. Max text = max_size - 10.
 */
size_t make_text_ndef(const char* text, unsigned char* buf, size_t max_size) {
    const size_t text_len = strlen(text);
    const size_t payload_len = 3 + text_len;      /* status(1) + "en"(2) + text */
    const size_t total = 2 + 4 + payload_len + 1; /* TLV(2) + NDEF hdr(4) + payload + 0xFE */

    if (total > max_size) {
        return 0;
    }

    size_t i = 0;
    buf[i++] = 0x03;                             /* TLV type: NDEF message */
    buf[i++] = (unsigned char)(4 + payload_len); /* TLV value length */
    buf[i++] = 0xD1;                             /* NDEF header: MB=1, ME=1, SR=1, TNF=1 */
    buf[i++] = 0x01;                             /* Type length = 1 */
    buf[i++] = (unsigned char)payload_len;       /* Payload length (SR=1, 1 byte) */
    buf[i++] = 'T';                              /* NDEF type: Text */
    buf[i++] = 0x02;                             /* Status: UTF-8, language code length 2 */
    buf[i++] = 'e';
    buf[i++] = 'n';
    // NOLINTNEXTLINE(bugprone-not-null-terminated-result) — NDEF payload, not a string
    memcpy(buf + i, text, text_len);
    i += text_len;
    buf[i++] = 0xFE; /* TLV terminator */

    return i;
}

size_t parse_text_ndef(const unsigned char* buf, unsigned int size, char* out, size_t out_size) {
    unsigned int pos = 0;
    if (pos + 1 >= size || buf[pos] != 0x03) {
        return 0;
    }
    pos++;
    unsigned int tlv_len = buf[pos];
    pos++;
    unsigned int end = pos + tlv_len;
    if (end > size) {
        end = size;
    }
    if (pos + 5 >= end) {
        return 0;
    }

    pos += 2; /* skip NDEF flags + type_len */
    pos++;    /* skip payload_len (SR=1) */
    if (buf[pos] != 'T') {
        return 0;
    }
    pos++; /* skip type 'T' */
    unsigned char status = buf[pos];
    pos++;
    unsigned int lang_len = status & 0x3F;
    pos += lang_len;

    unsigned int text_len = end > pos ? end - pos : 0;
    if (text_len >= out_size) {
        text_len = (unsigned int)out_size - 1;
    }
    memcpy(out, buf + pos, text_len);
    out[text_len] = '\0';
    return text_len;
}

int wsc_attr_to_json_kv(char* dest, size_t size, unsigned int attr_id, unsigned int attr_len,
                        const unsigned char* data) {
    const char* key = NULL;
    switch (attr_id) {
    case 0x1045:
        key = "ssid";
        break;
    case 0x1003:
        key = "auth_type";
        break;
    case 0x100F:
        key = "encryption_type";
        break;
    case 0x1027:
        key = "network_key";
        break;
    case 0x1020:
        key = "mac_address";
        break;
    default:
        return 0;
    }

    /* nosemgrep: raptor-unsafe-ret-snprintf-vsnprintf */
    size_t w = (size_t)snprintf(dest, size, "\"%s\": ", key);
    if (w >= size) {
        return -1;
    }

    if (attr_id == 0x1045 || attr_id == 0x1027) {
        /* nosemgrep: raptor-unsafe-ret-snprintf-vsnprintf */
        w += (size_t)snprintf(dest + w, size - w, "\"");
        for (unsigned int i = 0; i < attr_len; i++) {
            if (w + 6 >= size) {
                break;
            }
            if (data[i] == '"' || data[i] == '\\') {
                dest[w++] = '\\';
                dest[w++] = (char)data[i];
            } else if (data[i] >= 0x20 && data[i] <= 0x7E) {
                dest[w++] = (char)data[i];
            } else {
                w += (size_t)snprintf(dest + w, size - w, "\\u%04x", data[i]);
            }
        }
        if (w < size) {
            w += (size_t)snprintf(dest + w, size - w, "\"");
        }
    } else if (attr_id == 0x1020) {
        /* MAC Address: colon-separated hex */
        w += (size_t)snprintf(dest + w, size - w, "\"");
        for (unsigned int i = 0; i < attr_len; i++) {
            if (w + 4 >= size) {
                break;
            }
            if (i > 0) {
                dest[w++] = ':';
            }
            /* nosemgrep: raptor-unsafe-ret-snprintf-vsnprintf */
            w += (size_t)snprintf(dest + w, size - w, "%02x", data[i]);
        }
        if (w < size) {
            dest[w++] = '"';
        }
    } else {
        unsigned int val = 0;
        for (unsigned int i = 0; i < attr_len; i++) {
            val = (val << 8) | data[i];
        }
        w += (size_t)snprintf(dest + w, size - w, "%u", val);
    }

    dest[w < size ? w : size - 1] = '\0';
    return (int)w;
}

int iterate_ndef_attrs(const unsigned char* buf, unsigned int size, wsc_attr_fn cb, void* ctx) {
    unsigned int pos = 0;

    if (pos + 1 >= size || buf[pos] != 0x03) {
        return -1;
    }
    pos++;
    unsigned int tlv_len = buf[pos];
    pos++;
    unsigned int ndef_end = pos + tlv_len;
    if (ndef_end > size) {
        ndef_end = size;
    }

    if (pos + 2 >= ndef_end) {
        return -1;
    }
    unsigned char ndef_hdr = buf[pos];
    unsigned int type_len = buf[pos + 1];
    unsigned int sr = (ndef_hdr >> 4) & 1;
    unsigned int il = (ndef_hdr >> 3) & 1;
    pos += 2;

    unsigned int payload_len;
    if (sr) {
        payload_len = buf[pos];
        pos += 1;
    } else {
        payload_len = ((unsigned int)buf[pos] << 24) | ((unsigned int)buf[pos + 1] << 16) |
                      ((unsigned int)buf[pos + 2] << 8) | (unsigned int)buf[pos + 3];
        pos += 4;
    }

    if (il) {
        if (pos >= ndef_end) {
            return -1;
        }
        unsigned int id_len = buf[pos];
        pos += 1 + id_len;
    }

    pos += type_len;
    if (pos + payload_len > ndef_end) {
        payload_len = ndef_end - pos;
    }

    unsigned int payload_end = pos + payload_len;

    while (pos + 4 <= payload_end) {
        unsigned int attr_id = ((unsigned int)buf[pos] << 8) | (unsigned int)buf[pos + 1];
        unsigned int attr_len = ((unsigned int)buf[pos + 2] << 8) | (unsigned int)buf[pos + 3];
        pos += 4;

        if (pos + attr_len > payload_end) {
            break;
        }

        if (attr_id == 0x100E) {
            unsigned int cred_end = pos + attr_len;
            while (pos + 4 <= cred_end) {
                unsigned int sub_id = ((unsigned int)buf[pos] << 8) | (unsigned int)buf[pos + 1];
                unsigned int sub_len =
                    ((unsigned int)buf[pos + 2] << 8) | (unsigned int)buf[pos + 3];
                pos += 4;
                if (pos + sub_len > cred_end) {
                    break;
                }

                cb(ctx, sub_id, sub_len, buf + pos);
                pos += sub_len;
            }
            continue;
        }

        cb(ctx, attr_id, attr_len, buf + pos);
        pos += attr_len;
    }

    return 0;
}

struct json_ctx {
    int first;
};

static void json_attr_cb(void* ctx_v, unsigned int attr_id, unsigned int attr_len,
                         const unsigned char* data) {
    struct json_ctx* ctx = ctx_v;
    char jbuf[512];
    int r = wsc_attr_to_json_kv(jbuf, sizeof(jbuf), attr_id, attr_len, data);
    if (r > 0) {
        printf("%s  %s", ctx->first ? "" : ",\n", jbuf);
        ctx->first = 0;
    }
}

static int detect_record_type(const unsigned char* buf, unsigned int size, char* out_type,
                              size_t out_size) {
    /* TLV header (0x03 + length) */
    unsigned int pos = 0;
    if (pos + 1 >= size || buf[pos] != 0x03) {
        return -1;
    }
    pos++;
    unsigned int tlv_len = buf[pos];
    pos++;
    unsigned int end = pos + tlv_len;
    if (end > size) {
        end = size;
    }
    if (pos + 2 >= end) {
        return -1;
    }

    unsigned char ndef_hdr = buf[pos];
    unsigned int type_len = buf[pos + 1];
    unsigned int sr = (ndef_hdr >> 4) & 1;
    unsigned int il = (ndef_hdr >> 3) & 1;
    pos += 2;

    unsigned int payload_len;
    if (sr) {
        payload_len = buf[pos];
        pos += 1;
    } else {
        payload_len = ((unsigned int)buf[pos] << 24) | ((unsigned int)buf[pos + 1] << 16) |
                      ((unsigned int)buf[pos + 2] << 8) | (unsigned int)buf[pos + 3];
        pos += 4;
    }
    (void)payload_len;

    if (il) {
        if (pos >= end) {
            return -1;
        }
        unsigned int id_len = buf[pos];
        pos += 1 + id_len;
    }

    if (pos + type_len > end) {
        return -1;
    }

    if (type_len == 23 && memcmp(buf + pos, "application/vnd.wfa.wsc", 23) == 0) {
        snprintf(out_type, out_size, "wifi");
        return 0;
    }
    if (type_len == 1 && buf[pos] == 'U') {
        snprintf(out_type, out_size, "uri");
        return 0;
    }
    if (type_len == 1 && buf[pos] == 'T') {
        snprintf(out_type, out_size, "text");
        return 0;
    }
    snprintf(out_type, out_size, "unknown");
    return 0;
}

int fprint_ndef_json(const unsigned char* ndef_rbuf, unsigned int aligned_size) {
    char rectype[32];
    int rc = detect_record_type(ndef_rbuf, aligned_size, rectype, sizeof(rectype));
    if (rc != 0) {
        printf("{}\n");
        return rc;
    }

    if (strcmp(rectype, "wifi") == 0) {
        struct json_ctx ctx = {.first = 1};
        printf("{\n");
        iterate_ndef_attrs(ndef_rbuf, aligned_size, json_attr_cb, &ctx);
        printf("\n}\n");
        return 0;
    }

    if (strcmp(rectype, "uri") == 0) {
        unsigned int pos = 0;
        pos++; /* 0x03 */
        unsigned int tlv_len = ndef_rbuf[pos];
        pos++;
        unsigned int end = pos + tlv_len;
        if (end > aligned_size) {
            end = aligned_size;
        }
        pos += 2; /* ndef_hdr + type_len */
        pos += 3; /* payload_len + type ('U') + uri_id */
        unsigned int body_len = end - pos;
        printf("{\n  \"type\": \"uri\",\n  \"uri\": \"");
        /* Build URI from prefix id + body */
        unsigned char id = ndef_rbuf[pos - 1];
        if (id > 0 && id <= 23) {
            static const char* uri_prefixes[] = {
                "",           "http://www.", "https://www.", "http://",
                "https://",   "tel:",        "mailto:",      "ftp://anonymous:anonymous@",
                "ftp://ftp.", "ftps://",     "sftp://",      "smb://",
                "nfs://",     "ftp://",      "dav://",       "news:",
                "telnet://",  "imap:",       "rtsp://",      "urn:",
                "pop:",       "sip:",        "sips:",        "tftp:"};
            printf("%s", uri_prefixes[id]);
        }
        for (unsigned int i = 0; i < body_len && i < 200; i++) {
            unsigned char c = ndef_rbuf[pos + i];
            if (c >= 0x20 && c <= 0x7E) {
                putchar(c);
            }
        }
        printf("\"\n}\n");
        return 0;
    }

    if (strcmp(rectype, "text") == 0) {
        char text[256];
        size_t text_len = parse_text_ndef(ndef_rbuf, aligned_size, text, sizeof(text));
        printf("{\n  \"type\": \"text\",\n  \"text\": \"");
        for (size_t i = 0; i < text_len; i++) {
            unsigned char c = (unsigned char)text[i];
            if (c == '"' || c == '\\') {
                putchar('\\');
                putchar(c);
            } else if (c == '\n') {
                putchar('\\');
                putchar('n');
            } else if (c < 0x20) {
                /* skip control chars other than \n */
            } else {
                putchar(c);
            }
        }
        printf("\"\n}\n");
        return 0;
    }

    printf("{}\n");
    return 0;
}
