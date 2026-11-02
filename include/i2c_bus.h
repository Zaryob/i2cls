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

#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "i2c_types.h"

typedef struct i2c_bus_ctx {
    int bus_nr;
    int fd;
    char path[64];
    char adapter_name[128];
    unsigned long funcs;
    bool is_mock;
} i2c_bus_ctx_t;

/* Bus lifecycle */
i2c_bus_ctx_t *i2c_bus_open(int bus_nr, bool mock_mode);
void i2c_bus_close(i2c_bus_ctx_t *ctx);

/* Adapter functionality checking */
bool i2c_bus_has_func(i2c_bus_ctx_t *ctx, unsigned long func);
void i2c_bus_print_funcs(i2c_bus_ctx_t *ctx);

/* I/O probing & transfer operations */
i2c_probe_status_t i2c_bus_probe_addr(i2c_bus_ctx_t *ctx, uint8_t addr);

int i2c_bus_read_reg8(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t reg, uint8_t *out_val);
int i2c_bus_read_reg8_buf(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t reg, uint8_t *buf, size_t len);
int i2c_bus_write_read(i2c_bus_ctx_t *ctx, uint8_t addr,
                       const uint8_t *wbuf, size_t wlen,
                       uint8_t *rbuf, size_t rlen);
int i2c_bus_write_byte(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t val);
int i2c_bus_read_byte(i2c_bus_ctx_t *ctx, uint8_t addr, uint8_t *val);

#endif /* I2C_BUS_H */
