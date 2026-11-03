/* Identification regressions with injected register replies; no adapter access. */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "i2c_bus.h"
#include "i2c_db.h"

static size_t reads;
static int reply_length = 2;
static uint8_t manufacturer = 0x54, device = 0x04, bme_id = 0x60;

int i2c_bus_read_reg8_buf(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
    (void)ctx;
    reads++;
    if (reply_length < 0) return reply_length;
    if (addr == 0x76 && reg == 0xd0) {
        if (reply_length) buf[0] = bme_id;
        return reply_length ? 1 : 0;
    }
    if (addr != 0x18 || len != 2 || (reg != 0x06 && reg != 0x07)) return -ENXIO;
    if (reply_length > 0) buf[0] = reg == 0x06 ? 0 : device;
    if (reply_length > 1) buf[1] = reg == 0x06 ? manufacturer : 0;
    return reply_length;
}

int i2c_bus_write_read(i2c_bus_ctx_t *ctx, uint8_t addr, const uint8_t *wbuf,
                       size_t wlen, uint8_t *rbuf, size_t rlen)
{
    (void)ctx; (void)addr; (void)wbuf; (void)wlen; (void)rbuf; (void)rlen;
    assert(!"Unexpected command write during register identification");
    return -EIO;
}

int main(void)
{
    i2c_bus_ctx_t ctx = { .fd = -1, .is_mock = true };
    const i2c_chip_info_t *chip = i2c_db_identify_for_vendor(&ctx, 0x76, VENDOR_BOSCH);
    assert(chip && !strcmp(chip->name, "BME280") && chip->vendor_id == VENDOR_BOSCH);
    bme_id = 0xff;
    assert(!i2c_db_identify_for_vendor(&ctx, 0x76, VENDOR_BOSCH));
    reply_length = 0;
    assert(!i2c_db_identify_for_vendor(&ctx, 0x76, VENDOR_BOSCH));
    reply_length = -EIO;
    assert(!i2c_db_identify_for_vendor(&ctx, 0x76, VENDOR_BOSCH));
    reply_length = 2;
    chip = i2c_db_identify_for_vendor(&ctx, 0x18, VENDOR_MICROCHIP);
    assert(chip && !strcmp(chip->name, "MCP9808"));
    reply_length = 1;
    assert(!i2c_db_identify_for_vendor(&ctx, 0x18, VENDOR_MICROCHIP));
    reply_length = 2;
    manufacturer = 0x55;
    assert(!i2c_db_identify_for_vendor(&ctx, 0x18, VENDOR_MICROCHIP));
    manufacturer = 0x54;
    device = 0x05;
    assert(!i2c_db_identify_for_vendor(&ctx, 0x18, VENDOR_MICROCHIP));
    size_t before = reads;
    assert(!i2c_db_identify_for_vendor(&ctx, 0x76, VENDOR_TI));
    assert(!i2c_db_identify_for_vendor(&ctx, 0x76, VENDOR_COUNT));
    assert(!i2c_db_identify_for_vendor(&ctx, 0x50, VENDOR_MICROCHIP));
    assert(!i2c_db_identify_for_vendor(&ctx, 0x3c, VENDOR_SOLOMON));
    assert(reads == before); /* Wrong vendors and metadata-only chips cause no reads. */
    puts("Identification regressions passed (positive, negative, short and filtered replies).");
    return 0;
}
