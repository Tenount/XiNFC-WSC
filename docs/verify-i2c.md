# Verifying I2C on Xiaomi AX3000T

Step-by-step guide to verify NFC chip connectivity via I2C on AX3000T with OpenWrt.

## Prerequisites

- Xiaomi AX3000T with OpenWrt installed
- SSH access to the router
- Ethernet or Wi-Fi connection to the router

## Step 1: Connect via SSH

```bash
ssh root@192.168.1.1
```

Default IP is `192.168.1.1`. If changed, use the actual address.

## Step 2: Check I2C bus exists

```bash
ls /dev/i2c-*
```

Expected output:

```text
/dev/i2c-0
```

If `/dev/i2c-0` is missing, I2C kernel module is not loaded:

```bash
# Load I2C kernel modules
insmod /lib/modules/*/i2c-core.ko
insmod /lib/modules/*/i2c-mt7621.ko
```

## Step 3: Install i2c-tools

```bash
opkg update
opkg install i2c-tools
```

## Step 4: Scan I2C bus for devices

```bash
i2cdetect -y 0
```

Expected output — NFC chip at address `0x57`:

```text
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- 57 -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --                        
                      ^
                      |
               NFC chip (PN5180)
```

### Reading the table

| Value | Meaning |
|-------|---------|
| `57` | Device responding at address `0x57` |
| `--` | No device at this address |
| `UU` | Device claimed by kernel driver |

### What if `0x57` is not shown?

1. **Check wiring** — I2C SDA/SCL lines must be connected correctly
2. **Check pull-ups** — I2C requires pull-up resistors on SDA and SCL
3. **Check power** — NFC chip must be powered (3.3V)
4. **Try different bus** — `i2cdetect -y 1` for bus 1
5. **Check kernel logs** — `dmesg | grep i2c`

## Step 5: Check NFC chip presence (with standard i2c-tools)

Standard `i2c-tools` can detect the chip on the bus, but **cannot read NDEF data** — they use 8-bit register addressing while the NDEF memory requires 16-bit. Use our CLI instead.

### Check chip presence (works with standard tools)

```bash
# Scan I2C bus — shows device at 0x57
i2cdetect -y 0

# Read product ID register (8-bit addressing works here)
i2cget -y 0 0x57 0x00
# Output: 0x03  (PN5180 product ID)
```

If you see `0x03` at register 0x00 — chip is alive and responding.

### Read NDEF data (requires xinfc-wsc)

Standard tools use 8-bit addressing, but NDEF memory at offset 0x10 uses **16-bit addressing**. Read with our CLI:

```bash
# Full hex dump + parsed WSC attributes
xinfc-wsc --read
# or alias:
xinfc-wsc --dump
```

Example output (written NDEF with SSID `NewSSID`, password `newpass456`):

```text
0000: 03 52 d2 17 38 61 70 70 6c 69 63 61 74 69 6f 6e 
0010: 2f 76 6e 64 2e 77 66 61 2e 77 73 63 10 0e 00 34 
0020: 10 26 00 01 01 10 45 00 07 4e 65 77 53 53 49 44 
0030: 10 03 00 02 00 22 10 0f 00 02 00 08 10 27 00 0a 
0040: 6e 65 77 70 61 73 73 34 35 36 10 20 00 06 ff ff 
...

NDEF Message found:
  SSID (0x1045): NewSSID
  Network Key (0x1027): newpass456
  Auth Type (0x1003): 0x0022 (WPA2 Personal)
  Encryption Type (0x100F): 0x0008 (AES/CCMP)
```

> **Note:** Standard `i2cdump`/`i2cget` show zeros at 0x10+ because they use 8-bit addressing. The chip expects 16-bit register addresses (high byte + low byte). Our CLI handles this correctly.

### Read specific register (standard tools work for register 0x00)

```bash
# Read product ID (should be 0x03 for PN5180)
i2cget -y 0 0x57 0x00
# Output: 0x03
```

### Factory backup (stock credentials)

Real factory backup content:

```text
00000000  03 55 d2 17 3b 61 70 70  6c 69 63 61 74 69 6f 6e  |.U..;application|
00000010  2f 76 6e 64 2e 77 66 61  2e 77 73 63 10 0e 00 37  |/vnd.wfa.wsc...7|
00000020  10 26 00 01 01 10 45 00  0b 58 69 61 6f 6d 69 5f  |.&....E..Xiaomi_|
00000030  46 35 33 43 10 03 00 02  00 20 10 0f 00 02 00 08  |F53C..... ......|
00000040  10 27 00 09 31 32 33 31  32 33 31 32 33 10 20 00  |.'..123123123. .|
00000050  06 ff ff ff ff ff ff fe  64 64 66 61 10 20 00 06  |........ddfa. ..|
00000060  ff ff ff ff ff ff fe 00  00 00 00 00 00 00 00 00  |................|
```

Decoded:

- **Mime type**: `application/vnd.wfa.wsc` (WSC NDEF record)
- **SSID**: `Xiaomi_F53C`
- **Password**: `123123123`
- **Encryption**: WPA2 Personal (AES/CCMP) — `0x0008` + auth `0x0022`

## Constants in xinfc

After verification, the confirmed values are used as constants in the code:

```c
// i2c_nfc_device.h
#define NFC_I2C_BUS "0"      // /dev/i2c-0
#define NFC_CHIP_ADDR 0x57   // PN5180 NFC chip
```

These are **hardware constants** for AX3000T — not configurable via UCI.

## Troubleshooting

### "Permission denied" on i2cdetect

```bash
# Run as root
su
```

### "No such device" on /dev/i2c-0

```bash
# Check if I2C module is loaded
lsmod | grep i2c

# Load if missing
modprobe i2c-core
modprobe i2c-mt7621
```

### Multiple devices on same address

If `i2cdetect` shows `UU` at `0x57`, a kernel driver has claimed the device. This is normal if the NFC driver is loaded.
