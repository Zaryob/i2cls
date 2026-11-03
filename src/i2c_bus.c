/*
 * i2c-tools-ng - Modern, smart I2C diagnostic and interrogation suite
 * Copyright (C) 2026 i2c-tools-ng contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "i2c_bus.h"
#include "i2c_compat.h"
#include "i2c_sysfs.h"

/* Structure for mock simulated device response */
typedef struct {
    uint8_t addr;
    uint8_t reg;
    uint8_t data[4];
    size_t data_len;
} mock_reg_entry_t;

static const mock_reg_entry_t s_mock_device_regs[] = {
    /* Simulated BME280 at 0x76 (CHIP_ID 0xD0 = 0x60) */
    { .addr = 0x76, .reg = 0xD0, .data = {0x60}, .data_len = 1 },
    /* Simulated MPU-6050 at 0x68 (WHO_AM_I 0x75 = 0x68) */
    { .addr = 0x68, .reg = 0x75, .data = {0x68}, .data_len = 1 },
    /* Simulated SSD1306 at 0x3C (Status 0x00 = 0x03) */
    { .addr = 0x3C, .reg = 0x00, .data = {0x03}, .data_len = 1 },
    /* Simulated INA219 at 0x40 (Config 0x00 = 0x39, 0x9F) */
    { .addr = 0x40, .reg = 0x00, .data = {0x39, 0x9F}, .data_len = 2 },
    /* Simulated TCA9548A Multiplexer at 0x70 */
    { .addr = 0x70, .reg = 0x00, .data = {0x00}, .data_len = 1 }
};

static const size_t s_mock_reg_count = sizeof(s_mock_device_regs) / sizeof(s_mock_device_regs[0]);

i2c_bus_ctx_t *i2c_bus_open(int bus_nr, bool mock_mode)
{
    if (bus_nr < 0) { errno = EINVAL; return NULL; }
    i2c_bus_ctx_t *ctx = calloc(1, sizeof(i2c_bus_ctx_t));
    if (!ctx) return NULL;

    ctx->bus_nr = bus_nr;
    ctx->is_mock = mock_mode;
    snprintf(ctx->path, sizeof(ctx->path), "/dev/i2c-%d", bus_nr);

#if !defined(__linux__) || defined(I2C_FORCE_MOCK)
    if (!mock_mode) { free(ctx); errno = ENOSYS; return NULL; }
#endif

    if (ctx->is_mock) {
        if (bus_nr < 1 || bus_nr > 3) { free(ctx); errno = ENODEV; return NULL; }
        ctx->fd = -1;
        snprintf(ctx->adapter_name, sizeof(ctx->adapter_name), "Virtual/Mock I2C Adapter #%d", bus_nr);
        ctx->funcs = I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
        return ctx;
    }

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    ctx->fd = open(ctx->path, O_RDWR);
    if (ctx->fd < 0) {
        free(ctx);
        return NULL;
    }

    if (ioctl(ctx->fd, I2C_FUNCS, &ctx->funcs) < 0) {
        int saved = errno;
        close(ctx->fd);
        free(ctx);
        errno = saved;
        return NULL;
    }

    /* Try reading name from sysfs */
    char sysfs_name_path[256];
    snprintf(sysfs_name_path, sizeof(sysfs_name_path), I2C_SYSFS_ROOT "/i2c-%d/name", bus_nr);
    FILE *f = fopen(sysfs_name_path, "r");
    if (f) {
        if (fgets(ctx->adapter_name, sizeof(ctx->adapter_name), f)) {
            size_t len = strlen(ctx->adapter_name);
            if (len > 0 && ctx->adapter_name[len - 1] == '\n') {
                ctx->adapter_name[len - 1] = '\0';
            }
        }
        fclose(f);
    } else {
        snprintf(ctx->adapter_name, sizeof(ctx->adapter_name), "i2c-%d", bus_nr);
    }

    return ctx;
#else
    return ctx;
#endif
}

void i2c_bus_close(i2c_bus_ctx_t *ctx)
{
    if (!ctx) return;
    if (ctx->fd >= 0) {
        close(ctx->fd);
    }
    free(ctx);
}

bool i2c_bus_has_func(i2c_bus_ctx_t *ctx, unsigned long func)
{
    if (!ctx) return false;
    return (ctx->funcs & func) == func;
}

void i2c_bus_print_funcs(i2c_bus_ctx_t *ctx)
{
    if (!ctx) return;
    printf("Adapter Functionalities (0x%08lx):\n", ctx->funcs);
    printf("  - Plain I2C Transfer      : %s\n", (ctx->funcs & I2C_FUNC_I2C) ? "Yes" : "No");
    printf("  - 10-Bit Addressing       : %s\n", (ctx->funcs & I2C_FUNC_10BIT_ADDR) ? "Yes" : "No");
    printf("  - SMBus Quick Command     : %s\n", (ctx->funcs & I2C_FUNC_SMBUS_QUICK) ? "Yes" : "No");
    printf("  - SMBus Byte Data Read    : %s\n", (ctx->funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA) ? "Yes" : "No");
    printf("  - SMBus Word Data Read    : %s\n", (ctx->funcs & I2C_FUNC_SMBUS_READ_WORD_DATA) ? "Yes" : "No");
    printf("  - SMBus Block Data Read   : %s\n", (ctx->funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA) ? "Yes" : "No");
    printf("  - SMBus I2C Block Read    : %s\n", (ctx->funcs & I2C_FUNC_SMBUS_READ_I2C_BLOCK) ? "Yes" : "No");
}

/*
 * Safe address probing: checks if device ACKs
 */
i2c_probe_status_t i2c_bus_probe_addr(i2c_bus_ctx_t *ctx, uint8_t addr)
{
    if (!ctx || addr < 0x03 || addr > 0x77) return PROBE_ERROR;

    /* Check if kernel sysfs already has a driver bound to this address */
    char bound_driver[64] = {0};
    if (!ctx->is_mock && i2c_sysfs_is_driver_bound(ctx->bus_nr, addr, bound_driver, sizeof(bound_driver))) {
        return PROBE_ACK_BOUND;
    }

    if (ctx->is_mock) {
        i2c_sysfs_device_t device;
        if (!i2c_sysfs_get_device(ctx->bus_nr, addr, true, &device)) return PROBE_NO_ACK;
        return device.is_bound ? PROBE_ACK_BOUND : PROBE_ACK_UNBOUND;
    }

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    /* Set slave address */
    if (ioctl(ctx->fd, I2C_SLAVE, addr) < 0) {
        if (errno == EBUSY) {
            return PROBE_ACK_BOUND;
        }
        return PROBE_ERROR;
    }

    /* Use actual SMBus transactions, not a zero-length write(2). */
    union i2c_smbus_data data;
    struct i2c_smbus_ioctl_data request = {0};
    bool read_probe = (addr >= 0x30 && addr <= 0x37) ||
                      (addr >= 0x50 && addr <= 0x5f) ||
                      !(ctx->funcs & I2C_FUNC_SMBUS_QUICK);
    if (read_probe) {
        if (!(ctx->funcs & I2C_FUNC_SMBUS_READ_BYTE)) {
            errno = EOPNOTSUPP;
            return PROBE_ERROR;
        }
        request.read_write = I2C_SMBUS_READ;
        request.size = I2C_SMBUS_BYTE;
        request.data = &data;
    } else {
        request.read_write = I2C_SMBUS_WRITE;
        request.size = I2C_SMBUS_QUICK;
    }
    if (ioctl(ctx->fd, I2C_SMBUS, &request) == 0) return PROBE_ACK_UNBOUND;
    /* SMBus emulation on adapters such as bcm2835 may report NACK as EIO. */
    if (errno == ENXIO || errno == EREMOTEIO || errno == ENODEV || errno == EIO) return PROBE_NO_ACK;
    return PROBE_ERROR;
#else
    return PROBE_NO_ACK;
#endif
}

int i2c_bus_read_reg8(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t reg, uint8_t *out_val)
{
    return i2c_bus_read_reg8_buf(ctx, addr, reg, out_val, 1);
}

int i2c_bus_read_reg8_buf(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
    if (!ctx || !buf || len == 0 || len > 8192 || addr < 0x03 || addr > 0x77) return -EINVAL;

    if (ctx->is_mock) {
        i2c_sysfs_device_t device;
        if (!i2c_sysfs_get_device(ctx->bus_nr, addr, true, &device)) return -ENXIO;
        for (size_t i = 0; i < s_mock_reg_count; i++) {
            if (s_mock_device_regs[i].addr == addr && s_mock_device_regs[i].reg == reg) {
                size_t copy_len = len < s_mock_device_regs[i].data_len ? len : s_mock_device_regs[i].data_len;
                memcpy(buf, s_mock_device_regs[i].data, copy_len);
                return (int)copy_len;
            }
        }
        return -EIO;
    }

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    if (ioctl(ctx->fd, I2C_SLAVE, addr) < 0) {
        return -errno;
    }

    if (ctx->funcs & I2C_FUNC_I2C) {
        struct i2c_msg messages[2] = {
            { .addr = addr, .flags = 0, .len = 1, .buf = &reg },
            { .addr = addr, .flags = I2C_M_RD, .len = (uint16_t)len, .buf = buf }
        };
        struct i2c_rdwr_ioctl_data transfer = { .msgs = messages, .nmsgs = 2 };
        int ret = ioctl(ctx->fd, I2C_RDWR, &transfer);
        return ret < 0 ? -errno : (ret == 2 ? (int)len : -EIO);
    }
    if (len == 1 && (ctx->funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
        union i2c_smbus_data data;
        struct i2c_smbus_ioctl_data request = {
            .read_write = I2C_SMBUS_READ, .command = reg,
            .size = I2C_SMBUS_BYTE_DATA, .data = &data
        };
        if (ioctl(ctx->fd, I2C_SMBUS, &request) < 0) return -errno;
        buf[0] = data.byte;
        return 1;
    }
    return -EOPNOTSUPP;
#else
    return -ENOSYS;
#endif
}

int i2c_bus_write_read(i2c_bus_ctx_t *ctx, uint8_t addr,
                       const uint8_t *wbuf, size_t wlen,
                       uint8_t *rbuf, size_t rlen)
{
    if (!ctx || !wbuf || !rbuf || !wlen || !rlen || wlen > 8192 || rlen > 8192 || addr < 0x03 || addr > 0x77) return -EINVAL;

    if (ctx->is_mock) return -EIO;

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    if (ioctl(ctx->fd, I2C_SLAVE, addr) < 0) {
        return -errno;
    }

    if (write(ctx->fd, wbuf, wlen) != (ssize_t)wlen) {
        return -EIO;
    }

    ssize_t bytes_read = read(ctx->fd, rbuf, rlen);
    if (bytes_read < 0) {
        return -errno;
    }

    return (int)bytes_read;
#else
    return -ENOSYS;
#endif
}

int i2c_bus_write_byte(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t val)
{
    if (!ctx || addr < 0x03 || addr > 0x77) return -EINVAL;
    if (ctx->is_mock) {
        (void)addr;
        (void)val;
        return 0;
    }

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    if (ioctl(ctx->fd, I2C_SLAVE, addr) < 0) return -errno;
    return (write(ctx->fd, &val, 1) == 1) ? 0 : -EIO;
#else
    (void)addr;
    (void)val;
    return -ENOSYS;
#endif
}

int i2c_bus_read_byte(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t *val)
{
    if (!ctx || !val || addr < 0x03 || addr > 0x77) return -EINVAL;
    if (ctx->is_mock) {
        (void)addr;
        *val = 0;
        return 0;
    }

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    if (ioctl(ctx->fd, I2C_SLAVE, addr) < 0) return -errno;
    return (read(ctx->fd, val, 1) == 1) ? 0 : -EIO;
#else
    (void)addr;
    return -ENOSYS;
#endif
}
