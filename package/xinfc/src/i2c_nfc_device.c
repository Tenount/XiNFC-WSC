#include "i2c_nfc_device.h"

static unsigned int min_uint(unsigned int a, unsigned int b) {
    return (a < b) ? a : b;
}

static void i2c_set_error(struct i2c_nfc_device* dev, const char* msg, int eno, int ret) {
    dev->ret_code = 20;
    dev->error.msg = msg;
    dev->error.eno = eno;
    dev->error.ret = ret;
}

void i2c_close_device(struct i2c_nfc_device* dev) {
    if (dev->fd < 0) {
        return;
    }
    close(dev->fd);
    dev->fd = -1;
}

void i2c_check_error(struct i2c_nfc_device* dev) {
    if (dev->ret_code == 0) {
        return;
    }

    i2c_close_device(dev);
    fprintf(stderr, "xinfc: %s (ret=%d, errno=%d)\n", dev->error.msg, dev->error.ret,
            dev->error.eno);
    exit(dev->ret_code);
}

struct i2c_nfc_device i2c_make_device(const char* bus, unsigned short address) {
    struct i2c_nfc_device dev;
    dev.address = address;
    dev.ret_code = 0;
    dev.fd = -1;
    dev.error.msg = NULL;
    dev.error.eno = 0;
    dev.error.ret = 0;

#if !defined(XINFC_DUMMY_OUT)
    char bus_path[100];
    snprintf(bus_path, sizeof(bus_path), "/dev/i2c-%s", bus);
    dev.fd = open(bus_path, O_RDWR | O_CLOEXEC, 0);
    if (dev.fd < 0) {
        i2c_set_error(&dev, "failed to open I2C bus", errno, dev.fd);
    }
    i2c_check_error(&dev);
#else
    (void)bus;
#endif

    return dev;
}

void i2c_set_timeout(struct i2c_nfc_device* dev, unsigned long timeout) {
#if !defined(XINFC_DUMMY_OUT)
    if (ioctl(dev->fd, I2C_TIMEOUT, timeout) < 0) {
        i2c_set_error(dev, "failed to set I2C timeout", errno, -1);
    }
#endif
}

void i2c_set_retries(struct i2c_nfc_device* dev, unsigned long retries) {
#if !defined(XINFC_DUMMY_OUT)
    int r = ioctl(dev->fd, I2C_RETRIES, retries);
    if (r < 0) {
        i2c_set_error(dev, "failed to set I2C retries", errno, r);
    }
#endif
}

void i2c_set_device_address(struct i2c_nfc_device* dev) {
#if !defined(XINFC_DUMMY_OUT)
    int r = ioctl(dev->fd, I2C_SLAVE, (unsigned long)dev->address);
    if (r < 0) {
        i2c_set_error(dev, "failed to set I2C device address", errno, r);
    }
#endif
}

void i2c_read_ndef(struct i2c_nfc_device* dev, unsigned char* buf, unsigned int size) {
    if (size == 0 || buf == NULL) {
        return;
    }

    dev->ret_code = 0;

    if ((size % 4) != 0) {
        size = (((size - 1) / 4) + 1) * 4;
    }

    if (size > MAX_NDEF_BUF_SIZE) {
        size = MAX_NDEF_BUF_SIZE;
    }

    memset(buf, 0, size);

#if !defined(XINFC_DUMMY_OUT)
    unsigned char addr_buf[2] = {(BASE_NDEF_ADDR >> 8) & 0xFF, BASE_NDEF_ADDR & 0xFF};

    struct i2c_msg msgs[2];
    msgs[0].addr = dev->address;
    msgs[0].flags = 0;
    msgs[0].len = sizeof(addr_buf);
    msgs[0].buf = addr_buf;

    msgs[1].addr = dev->address;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = (__u16)size;
    msgs[1].buf = buf;

    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    int r = ioctl(dev->fd, I2C_RDWR, &rdwr);
    if (r != 2) {
        i2c_set_error(dev, "failed to read from I2C device", errno, r);
    }
#endif
}

void i2c_write_ndef_at(struct i2c_nfc_device* dev, const unsigned char* buf, unsigned int size,
                       unsigned short offset) {
    if (size == 0 || buf == NULL) {
        return;
    }

    if (size > MAX_NDEF_BUF_SIZE) {
        i2c_set_error(dev, "invalid ndef buffer size", 0, 0);
        return;
    }

    dev->ret_code = 0;

#if !defined(XINFC_DUMMY_OUT)
    unsigned int write_nmsgs = ((size - 1) / 4) + 1;

    struct i2c_msg msgs[write_nmsgs];
    struct i2c_rdwr_ioctl_data rdwr;

    unsigned char wbufs[40][6];
    /* nosemgrep: raptor-suspicious-assert */
    _Static_assert(MAX_NDEF_BUF_SIZE <= 40 * 4, "wbufs sized for MAX_NDEF_BUF_SIZE");

    for (unsigned int i = 0; i < write_nmsgs; i++) {
        unsigned int curr_buf_off = 4 * i;
        unsigned short ndef_addr = (unsigned short)(BASE_NDEF_ADDR + offset + curr_buf_off);
        unsigned int len = min_uint(size - curr_buf_off, 4U);

        wbufs[i][0] = (unsigned char)((ndef_addr >> 8) & 0xFF);
        wbufs[i][1] = (unsigned char)(ndef_addr & 0xFF);

        memcpy(wbufs[i] + 2, buf + curr_buf_off, len);
        memset(wbufs[i] + 2 + len, 0, 4 - len);

        msgs[i].addr = dev->address;
        msgs[i].flags = 0;
        msgs[i].len = 6;
        msgs[i].buf = wbufs[i];
    }

    rdwr.msgs = msgs;
    rdwr.nmsgs = write_nmsgs;

    int r = ioctl(dev->fd, I2C_RDWR, &rdwr);
    if (r != (int)write_nmsgs) {
        i2c_set_error(dev, "failed to write to I2C device", errno, r);
    }
#endif
}

struct i2c_nfc_device i2c_init_device(const char* bus, unsigned short address) {
    struct i2c_nfc_device dev = i2c_make_device(bus, address);
    i2c_set_timeout(&dev, 3);
    i2c_check_error(&dev);
    i2c_set_retries(&dev, 2);
    i2c_check_error(&dev);
    return dev;
}
