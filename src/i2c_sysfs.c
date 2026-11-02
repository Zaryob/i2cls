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
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "i2c_sysfs.h"

#define SYSFS_ADAPTER_DIR "/sys/class/i2c-adapter"
#define SYSFS_DEVICE_DIR  "/sys/bus/i2c/devices"

static void populate_mock_topology(i2c_topology_t *topo)
{
    topo->count = 3;

    /* Bus 1: Root Host Controller (e.g. BCM2835) */
    topo->adapters[0].bus_nr = 1;
    strncpy(topo->adapters[0].name, "bcm2835 I2C adapter (Broadcom SoC)", sizeof(topo->adapters[0].name));
    strncpy(topo->adapters[0].sysfs_path, "/sys/class/i2c-adapter/i2c-1", sizeof(topo->adapters[0].sysfs_path));
    topo->adapters[0].parent_bus_nr = -1;
    topo->adapters[0].is_mux_child = false;
    topo->adapters[0].num_devices = 3;

    topo->adapters[0].devices[0].addr = 0x3C;
    strncpy(topo->adapters[0].devices[0].name, "ssd1306", sizeof(topo->adapters[0].devices[0].name));
    topo->adapters[0].devices[0].is_bound = false;

    topo->adapters[0].devices[1].addr = 0x68;
    strncpy(topo->adapters[0].devices[1].name, "mpu6050", sizeof(topo->adapters[0].devices[1].name));
    strncpy(topo->adapters[0].devices[1].driver, "inv-mpu6050", sizeof(topo->adapters[0].devices[1].driver));
    topo->adapters[0].devices[1].is_bound = true;

    topo->adapters[0].devices[2].addr = 0x70;
    strncpy(topo->adapters[0].devices[2].name, "pca9548", sizeof(topo->adapters[0].devices[2].name));
    strncpy(topo->adapters[0].devices[2].driver, "pca954x", sizeof(topo->adapters[0].devices[2].driver));
    topo->adapters[0].devices[2].is_bound = true;

    /* Bus 2: MUX Channel 0 */
    topo->adapters[1].bus_nr = 2;
    strncpy(topo->adapters[1].name, "i2c-1-mux (chan_id 0)", sizeof(topo->adapters[1].name));
    strncpy(topo->adapters[1].sysfs_path, "/sys/class/i2c-adapter/i2c-2", sizeof(topo->adapters[1].sysfs_path));
    topo->adapters[1].parent_bus_nr = 1;
    topo->adapters[1].mux_channel = 0;
    topo->adapters[1].is_mux_child = true;
    topo->adapters[1].num_devices = 1;

    topo->adapters[1].devices[0].addr = 0x76;
    strncpy(topo->adapters[1].devices[0].name, "bme280", sizeof(topo->adapters[1].devices[0].name));
    topo->adapters[1].devices[0].is_bound = false;

    /* Bus 3: MUX Channel 1 */
    topo->adapters[2].bus_nr = 3;
    strncpy(topo->adapters[2].name, "i2c-1-mux (chan_id 1)", sizeof(topo->adapters[2].name));
    strncpy(topo->adapters[2].sysfs_path, "/sys/class/i2c-adapter/i2c-3", sizeof(topo->adapters[2].sysfs_path));
    topo->adapters[2].parent_bus_nr = 1;
    topo->adapters[2].mux_channel = 1;
    topo->adapters[2].is_mux_child = true;
    topo->adapters[2].num_devices = 1;

    topo->adapters[2].devices[0].addr = 0x40;
    strncpy(topo->adapters[2].devices[0].name, "ina219", sizeof(topo->adapters[2].devices[0].name));
    topo->adapters[2].devices[0].is_bound = false;
}

int i2c_sysfs_scan_topology(i2c_topology_t *topo, bool mock_mode)
{
    if (!topo) return -EINVAL;
    memset(topo, 0, sizeof(i2c_topology_t));

#if !defined(__linux__) || defined(I2C_FORCE_MOCK)
    mock_mode = true;
#endif

    if (mock_mode) {
        populate_mock_topology(topo);
        return 0;
    }

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    DIR *dir = opendir(SYSFS_ADAPTER_DIR);
    if (!dir) {
        /* Sysfs not mounted or no I2C adapters found */
        return -errno;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "i2c-", 4) != 0) {
            continue;
        }

        if (topo->count >= MAX_I2C_BUSES) {
            break;
        }

        int bus_nr = atoi(&entry->d_name[4]);
        i2c_adapter_info_t *ad = &topo->adapters[topo->count];
        ad->bus_nr = bus_nr;
        ad->parent_bus_nr = -1;
        ad->mux_channel = -1;
        ad->is_mux_child = false;

        snprintf(ad->sysfs_path, sizeof(ad->sysfs_path), "%s/%s", SYSFS_ADAPTER_DIR, entry->d_name);

        /* Read adapter name */
        char name_path[256];
        snprintf(name_path, sizeof(name_path), "%s/name", ad->sysfs_path);
        FILE *f = fopen(name_path, "r");
        if (f) {
            if (fgets(ad->name, sizeof(ad->name), f)) {
                size_t l = strlen(ad->name);
                if (l > 0 && ad->name[l - 1] == '\n') ad->name[l - 1] = '\0';
            }
            fclose(f);
        } else {
            snprintf(ad->name, sizeof(ad->name), "i2c-%d", bus_nr);
        }

        /* Check for MUX relationship via realpath */
        char real_path[1024];
        if (realpath(ad->sysfs_path, real_path)) {
            char *mux_pos = strstr(real_path, "channel-");
            if (mux_pos) {
                ad->is_mux_child = true;
                ad->mux_channel = atoi(mux_pos + 8);
                /* Search for parent i2c-X in real path */
                char *parent_i2c = strstr(real_path, "/i2c-");
                if (parent_i2c) {
                    ad->parent_bus_nr = atoi(parent_i2c + 5);
                }
            }
        }

        /* Scan devices instantiated on this adapter */
        char dev_pattern[32];
        snprintf(dev_pattern, sizeof(dev_pattern), "%d-00", bus_nr);
        DIR *dev_dir = opendir(SYSFS_DEVICE_DIR);
        if (dev_dir) {
            struct dirent *dentry;
            while ((dentry = readdir(dev_dir)) != NULL) {
                if (strncmp(dentry->d_name, dev_pattern, strlen(dev_pattern)) == 0) {
                    if (ad->num_devices >= 32) break;

                    uint8_t d_addr = (uint8_t)strtoul(&dentry->d_name[strlen(dev_pattern)], NULL, 16);
                    i2c_sysfs_device_t *dev = &ad->devices[ad->num_devices++];
                    dev->addr = d_addr;

                    /* Check driver binding */
                    char drv_link[256];
                    snprintf(drv_link, sizeof(drv_link), "%s/%s/driver", SYSFS_DEVICE_DIR, dentry->d_name);
                    char drv_target[256];
                    ssize_t link_len = readlink(drv_link, drv_target, sizeof(drv_target) - 1);
                    if (link_len > 0) {
                        drv_target[link_len] = '\0';
                        char *last_slash = strrchr(drv_target, '/');
                        strncpy(dev->driver, last_slash ? last_slash + 1 : drv_target, sizeof(dev->driver) - 1);
                        dev->is_bound = true;
                    }
                }
            }
            closedir(dev_dir);
        }

        topo->count++;
    }

    closedir(dir);
    return 0;
#else
    return 0;
#endif
}

bool i2c_sysfs_is_driver_bound(int bus_nr, uint8_t addr, char *out_driver, size_t driver_len)
{
#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    char dev_path[256];
    snprintf(dev_path, sizeof(dev_path), "%s/%d-%04x/driver", SYSFS_DEVICE_DIR, bus_nr, addr);
    
    char target[256];
    ssize_t ret = readlink(dev_path, target, sizeof(target) - 1);
    if (ret > 0) {
        target[ret] = '\0';
        if (out_driver && driver_len > 0) {
            char *slash = strrchr(target, '/');
            strncpy(out_driver, slash ? slash + 1 : target, driver_len - 1);
        }
        return true;
    }
    return false;
#else
    (void)bus_nr;
    (void)addr;
    (void)out_driver;
    (void)driver_len;
    return false;
#endif
}

int i2c_sysfs_bind_device(int bus_nr, uint8_t addr, const char *chip_name)
{
    if (!chip_name) return -EINVAL;

    char new_dev_path[256];
    snprintf(new_dev_path, sizeof(new_dev_path), "%s/i2c-%d/new_device", SYSFS_ADAPTER_DIR, bus_nr);

    FILE *f = fopen(new_dev_path, "w");
    if (!f) {
        return -errno;
    }

    int ret = fprintf(f, "%s 0x%02x\n", chip_name, addr);
    fclose(f);

    return (ret > 0) ? 0 : -EIO;
}

int i2c_sysfs_unbind_device(int bus_nr, uint8_t addr)
{
    char del_dev_path[256];
    snprintf(del_dev_path, sizeof(del_dev_path), "%s/i2c-%d/delete_device", SYSFS_ADAPTER_DIR, bus_nr);

    FILE *f = fopen(del_dev_path, "w");
    if (!f) {
        return -errno;
    }

    int ret = fprintf(f, "0x%02x\n", addr);
    fclose(f);

    return (ret > 0) ? 0 : -EIO;
}
