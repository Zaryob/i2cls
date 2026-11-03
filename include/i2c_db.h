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

#ifndef I2C_DB_H
#define I2C_DB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "i2c_types.h"
#include "i2c_vendor.h"

typedef enum {
    RULE_READ_REG8,     /* Write 8-bit register address, read 1-2 bytes */
    RULE_WRITE_READ,    /* Send multi-byte command, read response (e.g. SHTC3) */
    RULE_CUSTOM_CHECK   /* Custom verification handler */
} probe_rule_type_t;

typedef struct {
    probe_rule_type_t type;
    uint8_t reg_or_cmd[4];
    size_t cmd_len;
    uint8_t read_len;
    uint8_t expected_val;
    uint8_t mask;
    uint8_t read_offset;
} i2c_probe_rule_t;

typedef struct {
    const char *name;
    i2c_vendor_id_t vendor_id;
    i2c_category_t category;
    const char *description;
    const char *kernel_driver;
    const char *kernel_device; /* Exact I2C client name, distinct from kernel_driver. */
    uint8_t valid_addrs[16];  /* 0-terminated list of common 7-bit addresses */
    size_t num_rules;
    i2c_probe_rule_t rules[3];
} i2c_chip_info_t;

/* Forward declaration of bus context */
struct i2c_bus_ctx;

/* Database query and identification */
void i2c_db_init(void);
const i2c_chip_info_t *i2c_db_identify_device(struct i2c_bus_ctx *ctx, uint8_t addr);
/* Vendor filtering limits candidate signature reads; UNKNOWN means all vendors. */
const i2c_chip_info_t *i2c_db_identify_for_vendor(struct i2c_bus_ctx *ctx, uint8_t addr, i2c_vendor_id_t vendor);
/* Exact case-insensitive kernel device IDs only; never infer from module names. */
const i2c_chip_info_t *i2c_db_find_kernel_device(const char *name, uint8_t addr);
size_t i2c_db_get_total_chips(void);
const i2c_chip_info_t *i2c_db_get_chip_by_index(size_t index);

#endif /* I2C_DB_H */
