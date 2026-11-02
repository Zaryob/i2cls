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

#ifndef I2C_SYSFS_H
#define I2C_SYSFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_I2C_BUSES 64
#define MAX_BOUND_DEVS 128

typedef struct {
    uint8_t addr;
    char name[64];
    char driver[64];
    bool is_bound;
} i2c_sysfs_device_t;

typedef struct i2c_adapter_info {
    int bus_nr;
    char name[128];
    char sysfs_path[256];
    char parent_device[128];
    int parent_bus_nr;          /* -1 if root adapter, >=0 if child of a multiplexer */
    int mux_channel;            /* -1 if not mux channel, 0-7 if mux child */
    bool is_mux_child;
    
    size_t num_devices;
    i2c_sysfs_device_t devices[32];
} i2c_adapter_info_t;

typedef struct {
    size_t count;
    i2c_adapter_info_t adapters[MAX_I2C_BUSES];
} i2c_topology_t;

/* Sysfs discovery and information */
int i2c_sysfs_scan_topology(i2c_topology_t *topo, bool mock_mode);
bool i2c_sysfs_is_driver_bound(int bus_nr, uint8_t addr, char *out_driver, size_t driver_len);

/* Dynamic kernel driver instantiation via sysfs */
int i2c_sysfs_bind_device(int bus_nr, uint8_t addr, const char *chip_name);
int i2c_sysfs_unbind_device(int bus_nr, uint8_t addr);

#endif /* I2C_SYSFS_H */
