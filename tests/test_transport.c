/* Linux ioctl contract tests: no physical adapter is opened. */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "i2c_bus.h"
#include "i2c_compat.h"

static int select_error, transfer_error, transfer_result = 2;
static unsigned int last_size, last_direction, transfer_calls;
static unsigned long last_request;

int __wrap_ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    va_list ap;
    va_start(ap, request);
    last_request = request;
    if (request == I2C_SLAVE) {
        va_end(ap);
        if (select_error) { errno = select_error; return -1; }
        return 0;
    }
    transfer_calls++;
    if (request == I2C_SMBUS) {
        struct i2c_smbus_ioctl_data *args = va_arg(ap, struct i2c_smbus_ioctl_data *);
        last_size = args->size;
        last_direction = args->read_write;
        if (args->data) args->data->byte = 0x60;
    } else if (request == I2C_RDWR) {
        struct i2c_rdwr_ioctl_data *args = va_arg(ap, struct i2c_rdwr_ioctl_data *);
        assert(args->nmsgs == 2);
        assert(args->msgs[0].addr == 0x76 && args->msgs[1].addr == 0x76);
        assert(args->msgs[0].flags == 0 && args->msgs[1].flags == I2C_M_RD);
        assert(args->msgs[0].len == 1 && args->msgs[0].buf[0] == 0xd0);
        memset(args->msgs[1].buf, 0x60, args->msgs[1].len);
    } else assert(!"Unexpected ioctl (including forced slave access)");
    va_end(ap);
    if (transfer_error) { errno = transfer_error; return -1; }
    return request == I2C_RDWR ? transfer_result : 0;
}

int main(void)
{
    i2c_bus_ctx_t ctx = { .bus_nr = 99998, .fd = -1,
        .funcs = I2C_FUNC_I2C | I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_READ_BYTE };
    assert(i2c_bus_probe_addr(&ctx, 0x20) == PROBE_ACK_UNBOUND);
    assert(last_request == I2C_SMBUS && last_size == I2C_SMBUS_QUICK);
    assert(last_direction == I2C_SMBUS_WRITE);
    assert(i2c_bus_probe_addr(&ctx, 0x50) == PROBE_ACK_UNBOUND);
    assert(last_size == I2C_SMBUS_BYTE && last_direction == I2C_SMBUS_READ);
    assert(i2c_bus_probe_addr(&ctx, 0x30) == PROBE_ACK_UNBOUND);
    assert(last_size == I2C_SMBUS_BYTE);
    transfer_error = EIO;
    assert(i2c_bus_probe_addr(&ctx, 0x51) == PROBE_NO_ACK);
    transfer_error = ENXIO;
    assert(i2c_bus_probe_addr(&ctx, 0x20) == PROBE_NO_ACK);
    transfer_error = ETIMEDOUT;
    assert(i2c_bus_probe_addr(&ctx, 0x20) == PROBE_ERROR);
    transfer_error = 0;
    unsigned calls = transfer_calls;
    select_error = EBUSY;
    assert(i2c_bus_probe_addr(&ctx, 0x20) == PROBE_ACK_BOUND);
    assert(transfer_calls == calls);
    uint8_t value = 0;
    assert(i2c_bus_read_reg8(&ctx, 0x76, 0xd0, &value) == -EBUSY);
    assert(transfer_calls == calls);
    select_error = 0;
    assert(i2c_bus_read_reg8(&ctx, 0x76, 0xd0, &value) == 1 && value == 0x60);
    transfer_result = 1;
    assert(i2c_bus_read_reg8(&ctx, 0x76, 0xd0, &value) == -EIO);
    ctx.funcs = I2C_FUNC_SMBUS_READ_BYTE_DATA;
    assert(i2c_bus_read_reg8(&ctx, 0x76, 0xd0, &value) == 1 && value == 0x60);
    assert(last_size == I2C_SMBUS_BYTE_DATA);
    ctx.funcs = 0;
    calls = transfer_calls;
    assert(i2c_bus_probe_addr(&ctx, 0x20) == PROBE_ERROR && errno == EOPNOTSUPP);
    assert(i2c_bus_read_reg8(&ctx, 0x76, 0xd0, &value) == -EOPNOTSUPP);
    assert(transfer_calls == calls);
    puts("Linux transport contracts passed (no hardware access).");
    return 0;
}
