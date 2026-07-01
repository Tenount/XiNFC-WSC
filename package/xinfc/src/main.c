#include "i2c_nfc_device.h"
#include "version.h"
#include "wifi.h"
#include "xinfc-utils.h"

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#define LOCKFILE "/var/run/xinfc-wsc.lock"
static int g_lock_fd = -1;

static void release_lock(void) {
    if (g_lock_fd >= 0) {
        close(g_lock_fd);
        g_lock_fd = -1;
    }
}

static int acquire_lock(void) {
    int fd = open(LOCKFILE, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (fd < 0) {
        fprintf(stderr, "xinfc: cannot open lockfile %s\n", LOCKFILE);
        return -1;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) < 0) { /* nosemgrep: raptor-typos */
        if (errno == EWOULDBLOCK) {
            fprintf(stderr, "xinfc: another instance is running\n");
        } else {
            fprintf(stderr, "xinfc: flock failed (%d)\n", errno);
        }
        close(fd);
        return -1;
    }

    char pid_str[16];
    int len = snprintf(pid_str, sizeof(pid_str), "%ld\n", (long)getpid());
    if (ftruncate(fd, 0) < 0 || lseek(fd, 0, SEEK_SET) < 0 || write(fd, pid_str, (size_t)len) < 0) {
        fprintf(stderr, "xinfc: failed to write PID to lock file (%d)\n", errno);
        close(fd);
        return -1;
    }

    g_lock_fd = fd;
    return 0;
}

static void print_usage(const char* prog);

int main(int argc, char* argv[]) {
    int opt_read = 0;
    int opt_json = 0;
    int opt_clear = 0;
    int opt_scan = 0;
    const char* opt_write_uri = NULL;
    const char* opt_write_text = NULL;

    static struct option long_options[] = {{"read", no_argument, 0, 'r'},
                                           {"json", no_argument, 0, 'j'},
                                           {"clear", no_argument, 0, 'c'},
                                           {"scan", no_argument, 0, 's'},
                                           {"write-url", required_argument, 0, 'u'},
                                           {"write-text", required_argument, 0, 't'},
                                           {"help", no_argument, 0, 'h'},
                                           {"version", no_argument, 0, 'V'},
                                           {0, 0, 0, 0}};

    int c;
    while ((c = getopt_long(argc, argv, "hV", long_options, NULL)) != -1) {
        switch (c) {
        case 'r':
            opt_read = 1;
            break;
        case 'j':
            opt_json = 1;
            break;
        case 'c':
            opt_clear = 1;
            break;
        case 's':
            opt_scan = 1;
            break;
        case 'u':
            opt_write_uri = optarg;
            break;
        case 't':
            opt_write_text = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(0);
        case 'V':
            printf("xinfc %s\n"
                   "Copyright (C) %s %s\n"
                   "License GPLv3: GNU GPL version 3 <https://gnu.org/licenses/gpl.html>\n"
                   "This is free software: you are free to change and redistribute it.\n"
                   "There is NO WARRANTY, to the extent permitted by law.\n",
                   XINFC_VERSION, XINFC_COPYRIGHT_YEAR, XINFC_COPYRIGHT_HOLDER);
            exit(0);
        case '?':
            exit(1);
        }
    }

    if (opt_read) {
        return read_ndef(opt_json ? 1 : 0);
    }

    if (opt_scan) {
        return scan_i2c_bus();
    }

    if (opt_clear) {
        if (acquire_lock() != 0) {
            exit(30);
        }
        atexit(release_lock);
        return clear_ndef();
    }

    if (opt_write_uri) {
        if (acquire_lock() != 0) {
            exit(30);
        }
        atexit(release_lock);
        return write_ndef_uri(opt_write_uri);
    }

    if (opt_write_text) {
        if (acquire_lock() != 0) {
            exit(30);
        }
        atexit(release_lock);
        return write_ndef_text(opt_write_text);
    }

    if (argc - optind != 3) {
        print_usage(argv[0]);
        exit(1);
    }

    if (acquire_lock() != 0) {
        exit(30);
    }
    atexit(release_lock);

    const char* ssid = argv[optind];
    const char* password = argv[optind + 1];
    const char* mode = argv[optind + 2];

    const size_t ssid_len = strlen(ssid);
    if (ssid_len < SSID_MIN || ssid_len > SSID_MAX) {
        fprintf(stderr, "xinfc: ssid must have between %d and %d characters\n", SSID_MIN, SSID_MAX);
        exit(3);
    }

    enum wifi_crypt crypt;
    enum wifi_auth auth;

    if (select_encryption_mode(mode, &crypt, &auth) != 0) {
        exit(5);
    }

    if (crypt != wifi_crypt_none) {
        const size_t pass_len = strlen(password);
        if (pass_len < PASS_MIN || pass_len > PASS_MAX) {
            fprintf(stderr, "xinfc: password must have between %d and %d characters\n", PASS_MIN,
                    PASS_MAX);
            exit(4);
        }
    }

    return apply_config(ssid, password, crypt, auth);
}

static void print_usage(const char* prog) {
    fprintf(stderr,
            "USAGE: %s ssid password mode\n"
            "       %s --read\n"
            "       %s --read --json\n"
            "       %s --clear\n"
            "       %s --scan\n"
            "       %s --write-url <url>\n"
            "       %s --write-text <message>\n"
            "       %s -h | --help\n"
            "       %s -V | --version\n"
            "  ssid must have between %d and %d characters\n"
            "  password must have between %d and %d characters.\n"
            "  mode accepts OpenWrt-style names (e.g. psk2+aes, sae):\n",
            prog, prog, prog, prog, prog, prog, prog, prog, prog, SSID_MIN, SSID_MAX, PASS_MIN,
            PASS_MAX);

    fprintf(stderr, "    Open/OWE:      none, owe\n"
                    "    WEP:           wep, wep+open, wep+shared\n"
                    "    WPA1 PSK:      psk, psk+*, psk-mixed+*\n"
                    "    WPA2 PSK:      psk2, psk2+*, psk-mixed+*\n"
                    "    WPA3:          sae, sae-mixed\n"
                    "    WPA1 Ent:      wpa, wpa+*, wpa-mixed+*\n"
                    "    WPA2 Ent:      wpa2, wpa2+*, wpa-mixed+*\n"
                    "    WPA3 Ent:      wpa3, wpa3-mixed\n");
}