#pragma once

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_NDEF_BUF_SIZE 160
#define BASE_NDEF_ADDR 0x10
#define NFC_CHIP_ADDR 0x57
#define NFC_I2C_BUS "0"

struct i2c_error {
    const char* msg;
    int eno;
    int ret;
};

struct i2c_nfc_device {
    unsigned short address;
    int fd;
    int ret_code;
    struct i2c_error error;
};

struct i2c_nfc_device i2c_make_device(const char* bus, unsigned short address);
void i2c_close_device(struct i2c_nfc_device* dev);
void i2c_check_error(struct i2c_nfc_device* dev);
void i2c_set_timeout(struct i2c_nfc_device* dev, unsigned long timeout);
void i2c_set_retries(struct i2c_nfc_device* dev, unsigned long retries);
void i2c_set_device_address(struct i2c_nfc_device* dev);
void i2c_read_ndef(struct i2c_nfc_device* dev, unsigned char* buf, unsigned int size);
void i2c_write_ndef_at(struct i2c_nfc_device* dev, const unsigned char* buf, unsigned int size,
                       unsigned short offset);
struct i2c_nfc_device i2c_init_device(const char* bus, unsigned short address);
