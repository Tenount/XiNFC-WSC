#include "i2c_nfc_device.h"
#include "version.h"
#include "wifi.h"
#include "xinfc-utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static unsigned char uri_id_code(const char* uri);
static const char* uri_body(const char* uri, size_t* len);

int apply_config(const char* ssid, const char* password, enum wifi_crypt crypt,
                 enum wifi_auth auth) {
    fprintf(stderr, "Opening I2C bus %s...\n", NFC_I2C_BUS);

    __attribute__((cleanup(i2c_close_device))) struct i2c_nfc_device dev =
        i2c_init_device(NFC_I2C_BUS, NFC_CHIP_ADDR);

    fprintf(stderr, "Setting I2C timeout...\n");
    i2c_set_timeout(&dev, 3);
    i2c_check_error(&dev);

    fprintf(stderr, "Setting I2C retries...\n");
    i2c_set_retries(&dev, 2);
    i2c_check_error(&dev);

    fprintf(stderr, "Setting I2C device address %d...\n", NFC_CHIP_ADDR);
    i2c_set_device_address(&dev);
    i2c_check_error(&dev);
    fprintf(stderr, "I2C device address set.\n");

    unsigned char ndef_rbuf[MAX_NDEF_BUF_SIZE];
    unsigned char ndef_wbuf[MAX_NDEF_BUF_SIZE];

    fprintf(stderr, "Reading existing NDEF data...\n");
    i2c_read_ndef(&dev, ndef_rbuf, sizeof(ndef_rbuf));
    i2c_check_error(&dev);

    fprintf(stderr, "Read %zu bytes.\n", sizeof(ndef_rbuf));

    /* Build new NDEF */
    fprintf(stderr, "Building new NDEF data...\n");
    memset(ndef_wbuf, 0, sizeof(ndef_wbuf));
    const size_t size = make_wsc_ndef(ssid, password, crypt, auth, ndef_wbuf, sizeof(ndef_wbuf));
    if (size == 0) {
        fprintf(stderr, "xinfc: failed to build NDEF data\n");
        exit(13);
    }
    fprintf(stderr, "New NDEF is %zu bytes.\n", size);

    const unsigned int aligned_size = (unsigned int)((((size - 1) / 4) + 1) * 4);

    /* Skip write if data is identical to current (prevents EEPROM wear) */
    if (memcmp(ndef_rbuf, ndef_wbuf, aligned_size) == 0) {
        fprintf(stderr, "NDEF data unchanged, skipping write.\n");
        return 0;
    }

    /* Write with retries - chunked with delays */
    const int max_retries = 5;
    int retries = 0;

    for (; retries < max_retries; ++retries) {
        fprintf(stderr, "Writing new NDEF data...\n");

        const size_t max_bytes = 4;
        for (size_t i = 0; i < size; i += max_bytes) {
            const unsigned int chunk = (unsigned int)MIN(max_bytes, size - i);
            const unsigned int max_w_retries = 5;
            unsigned int w_retries = 0;

            for (; w_retries <= max_w_retries; ++w_retries) {
                i2c_write_ndef_at(&dev, ndef_wbuf + i, chunk, (unsigned short)i);
                if (dev.ret_code == 0) {
                    break;
                }
                if (w_retries == max_w_retries) {
                    fprintf(stderr, "\n");
                    exit(dev.ret_code);
                }
                fprintf(stderr, "x");
                struct timespec ts_retry = {0, 20000000};
                nanosleep(&ts_retry, NULL);
            }
            fprintf(stderr, ".");
        }

        fprintf(stderr, "\n");
        struct timespec ts_wait = {2, 0};
        nanosleep(&ts_wait, NULL);

        /* Verify written data */
        fprintf(stderr, "Verifying written NDEF data...\n");
        {
            memset(ndef_rbuf, 0, size);
            const unsigned int max_r_retries = 30;
            unsigned int r_retries = 0;

            for (; r_retries <= max_r_retries; ++r_retries) {
                i2c_read_ndef(&dev, ndef_rbuf, aligned_size);
                if (dev.ret_code == 0) {
                    break;
                }
                if (r_retries == max_r_retries) {
                    fprintf(stderr, "\n");
                    exit(20);
                }
                struct timespec ts_verify = {0, 100000000};
                nanosleep(&ts_verify, NULL);
            }

            if (memcmp(ndef_wbuf, ndef_rbuf, aligned_size) == 0) {
                fprintf(stderr, "Success.\n");
                fprint_ndef_json(ndef_rbuf, aligned_size);
                break;
            }
        }

        fprintf(stderr, "Data does not match! Retrying...\n");
    }

    if (retries == max_retries) {
        fprintf(stderr, "Error: failed to write new NDEF data\n");
        exit(20);
    }
    return 0;
}

static void print_wsc_attr(FILE* out, const unsigned char* data, unsigned int attr_id,
                           unsigned int attr_len) {
    const char* attr_name = "Unknown";
    switch (attr_id) {
    case 0x100E:
        attr_name = "Credential";
        break;
    case 0x1026:
        attr_name = "Network Index";
        break;
    case 0x1045:
        attr_name = "SSID";
        break;
    case 0x1003:
        attr_name = "Auth Type";
        break;
    case 0x100F:
        attr_name = "Encryption Type";
        break;
    case 0x1027:
        attr_name = "Network Key";
        break;
    case 0x1020:
        attr_name = "MAC Address";
        break;
    }

    if (attr_id == 0x1045 || attr_id == 0x1027) {
        fprintf(out, "%s: ", attr_name);
        for (unsigned int i = 0; i < attr_len; i++) {
            if (data[i] >= 0x20 && data[i] <= 0x7E) {
                fprintf(out, "%c", data[i]);
            } else {
                fprintf(out, ".");
            }
        }
        fprintf(out, "\n");
    } else if (attr_id != 0x100E && attr_id != 0x1026) {
        fprintf(out, "%s: len=%u (0x%04X)\n", attr_name, attr_len, attr_id);
    }
}

struct print_ctx {
    FILE* out;
};

static void print_attr_cb(void* ctx_v, unsigned int attr_id, unsigned int attr_len,
                          const unsigned char* data) {
    struct print_ctx* ctx = ctx_v;
    print_wsc_attr(ctx->out, data, attr_id, attr_len);
}

int read_ndef(int json_mode) {
    /* Suppress debug logging in JSON mode */
    if (!json_mode) {
        fprintf(stderr, "I2C device address is %d.\n", NFC_CHIP_ADDR);
    }

    __attribute__((cleanup(i2c_close_device))) struct i2c_nfc_device dev =
        i2c_init_device(NFC_I2C_BUS, NFC_CHIP_ADDR);

    if (!json_mode) {
        fprintf(stderr, "Setting I2C timeout...\n");
    }
    i2c_set_timeout(&dev, 3);
    i2c_check_error(&dev);

    if (!json_mode) {
        fprintf(stderr, "Setting I2C retries...\n");
    }
    i2c_set_retries(&dev, 2);
    i2c_check_error(&dev);

    if (!json_mode) {
        fprintf(stderr, "Setting I2C device address %d...\n", NFC_CHIP_ADDR);
        i2c_set_device_address(&dev);
        i2c_check_error(&dev);
        fprintf(stderr, "I2C device address set.\n");
    } else {
        i2c_set_device_address(&dev);
        i2c_check_error(&dev);
    }

    unsigned char ndef_rbuf[MAX_NDEF_BUF_SIZE];
    const unsigned int aligned_size = MAX_NDEF_BUF_SIZE;

    if (!json_mode) {
        fprintf(stderr, "Reading NDEF data...\n");
    }
    i2c_read_ndef(&dev, ndef_rbuf, aligned_size);
    i2c_check_error(&dev);

    if (!json_mode) {
        fprintf(stderr, "Read %u bytes.\n", aligned_size);

        /* Print raw hex dump */
        for (unsigned int i = 0; i < aligned_size; i++) {
            if (i % 16 == 0) {
                fprintf(stderr, "\n%04x: ", i);
            }
            fprintf(stderr, "%02x ", ndef_rbuf[i]);
        }
        fprintf(stderr, "\n");
    }

    /* Parse NDEF from NFC Type 2 Tag TLV format */
    if (json_mode) {
        return fprint_ndef_json(ndef_rbuf, aligned_size);
    }

    struct print_ctx pctx = {stdout};
    iterate_ndef_attrs(ndef_rbuf, aligned_size, print_attr_cb, &pctx);
    return 0;
}

int clear_ndef(void) {
    unsigned char empty[MAX_NDEF_BUF_SIZE];
    memset(empty, 0, sizeof(empty));
    fprintf(stderr, "Clearing NFC chip...\n");
    int rc = write_buffer(empty, MAX_NDEF_BUF_SIZE);
    if (rc == 0) {
        fprintf(stderr, "NFC chip cleared.\n");
    }
    return rc;
}

int write_buffer(const unsigned char* buf, unsigned int aligned_size) {
    unsigned char verify_buf[MAX_NDEF_BUF_SIZE];
    __attribute__((cleanup(i2c_close_device))) struct i2c_nfc_device dev =
        i2c_init_device(NFC_I2C_BUS, NFC_CHIP_ADDR);
    i2c_set_device_address(&dev);
    i2c_check_error(&dev);

    for (int retries = 0; retries < 5; ++retries) {
        for (size_t i = 0; i < aligned_size; i += 4) {
            unsigned int w_retries = 0;
            for (; w_retries <= 5; ++w_retries) {
                i2c_write_ndef_at(&dev, buf + i, 4, (unsigned short)i);
                if (dev.ret_code == 0) {
                    break;
                }
                if (w_retries == 5) {
                    exit(dev.ret_code);
                }
                struct timespec ts = {0, 20000000};
                nanosleep(&ts, NULL);
            }
        }
        memset(verify_buf, 0, sizeof(verify_buf));
        unsigned int r_retries = 0;
        for (; r_retries <= 30; ++r_retries) {
            i2c_read_ndef(&dev, verify_buf, aligned_size);
            if (dev.ret_code == 0) {
                break;
            }
            if (r_retries == 30) {
                exit(20);
            }
            struct timespec ts = {0, 100000000};
            nanosleep(&ts, NULL);
        }
        if (memcmp(buf, verify_buf, aligned_size) == 0) {
            return 0;
        }
        struct timespec ts = {2, 0};
        nanosleep(&ts, NULL);
    }
    return 20;
}

int write_ndef_uri(const char* uri) {
    const unsigned char uri_id = uri_id_code(uri);
    size_t body_len;
    const char* body = uri_body(uri, &body_len);
    if (!body) {
        return 13;
    }

    unsigned char buf[MAX_NDEF_BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    /* TL + NDEF header + Uri Type + payload_len + uri_id + body + terminator */
    size_t payload_len = 1 + (size_t)body_len;
    size_t total = 2 + 5 + payload_len + 1; /* TLV header+NDEF header+payload+0xFE */
    if (total > MAX_NDEF_BUF_SIZE) {
        return 13;
    }

    size_t i = 0;
    buf[i++] = 0x03;
    buf[i++] = (unsigned char)(4 + payload_len);
    buf[i++] = 0xD1; /* MB=1, ME=1, SR=1, TNF=1 (NFC Forum Well-known) */
    buf[i++] = 0x01; /* Type length */
    buf[i++] = (unsigned char)payload_len;
    buf[i++] = 'U'; /* URI type */
    buf[i++] = uri_id;
    memcpy(buf + i, body, (size_t)body_len);
    i += (size_t)body_len;
    buf[i++] = 0xFE; /* TLV terminator */

    unsigned int aligned = (unsigned int)((((i - 1) / 4) + 1) * 4);
    fprintf(stderr, "Writing URI NDEF...\n");
    int rc = write_buffer(buf, aligned);
    if (rc == 0) {
        fprintf(stderr, "URI written.\n");
    }
    return rc;
}

int write_ndef_text(const char* text) {
    unsigned char buf[MAX_NDEF_BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    const size_t size = make_text_ndef(text, buf, sizeof(buf));
    if (size == 0) {
        fprintf(stderr, "xinfc: text too long (max 150 bytes in UTF-8)\n");
        return 13;
    }
    const unsigned int aligned = (unsigned int)((((size - 1) / 4) + 1) * 4);
    fprintf(stderr, "Writing text NDEF...\n");
    const int rc = write_buffer(buf, aligned);
    if (rc == 0) {
        fprintf(stderr, "Text written.\n");
    }
    return rc;
}

int scan_i2c_bus(void) {
    __attribute__((cleanup(i2c_close_device))) struct i2c_nfc_device dev =
        i2c_init_device(NFC_I2C_BUS, 0x50);
    fprintf(stderr, "Scanning I2C bus %s for NFC chips...\n", NFC_I2C_BUS);
    int found = 0;
    for (unsigned short addr = 0x50; addr <= 0x57; addr++) {
        dev.address = addr;
        dev.ret_code = 0;
        i2c_set_device_address(&dev);
        if (dev.ret_code != 0) {
            continue;
        }
        /* try reading a single byte to confirm device presence */
        unsigned char probe[4];
        i2c_read_ndef(&dev, probe, 4);
        if (dev.ret_code == 0) {
            fprintf(stderr, "  found device at 0x%02x\n", addr);
            found = 1;
        }
    }
    if (!found) {
        fprintf(stderr, "  no NFC chip found on bus %s\n", NFC_I2C_BUS);
        return 3;
    }
    return 0;
}

static unsigned char uri_id_code(const char* uri) {
    if (strncmp(uri, "https://", 8) == 0) {
        return 0x04;
    }
    if (strncmp(uri, "http://", 7) == 0) {
        return 0x03;
    }
    return 0x00;
}

static const char* uri_body(const char* uri, size_t* len) {
    static const char* prefixes[] = {"https://", "http://", NULL};
    for (int p = 0; prefixes[p]; p++) {
        size_t plen = strlen(prefixes[p]);
        if (strncmp(uri, prefixes[p], plen) == 0) {
            *len = strlen(uri) - plen;
            return uri + plen;
        }
    }
    *len = strlen(uri);
    return uri;
}
