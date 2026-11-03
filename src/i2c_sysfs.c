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
#include <limits.h>
#include <ctype.h>
#include "i2c_types.h"

#include "i2c_sysfs.h"

#define SYSFS_ADAPTER_DIR I2C_SYSFS_ROOT
#define SYSFS_DEVICE_DIR I2C_SYSFS_ROOT

static const i2c_adapter_info_t s_mock_adapters[] = {
    { .bus_nr = 1, .name = "Mock root I2C adapter", .sysfs_path = "mock/i2c-1",
      .parent_bus_nr = -1, .mux_channel = -1, .num_devices = 5,
      .devices = {
          { .addr = 0x3c, .name = "oled" },
          { .addr = 0x40, .name = "ina219" },
          { .addr = 0x68, .name = "mpu6050", .driver = "inv-mpu6050", .is_bound = true },
          { .addr = 0x70, .name = "pca9548", .driver = "pca954x", .is_bound = true },
          { .addr = 0x76, .name = "bme280" }
      } },
    { .bus_nr = 2, .name = "i2c-1-mux (chan_id 0)", .sysfs_path = "mock/i2c-2",
      .parent_bus_nr = 1, .mux_channel = 0, .is_mux_child = true,
      .parent_device = "1-0070", .num_devices = 1,
      .devices = { { .addr = 0x76, .name = "bme280" } } },
    { .bus_nr = 3, .name = "i2c-1-mux (chan_id 1)", .sysfs_path = "mock/i2c-3",
      .parent_bus_nr = 1, .mux_channel = 1, .is_mux_child = true,
      .parent_device = "1-0070", .num_devices = 1,
      .devices = { { .addr = 0x40, .name = "ina219" } } }
};

static void populate_mock_topology(i2c_topology_t *topo)
{
    topo->count = sizeof(s_mock_adapters) / sizeof(s_mock_adapters[0]);
    memcpy(topo->adapters, s_mock_adapters, sizeof(s_mock_adapters));
}

bool i2c_sysfs_get_device(int bus_nr, uint8_t addr, bool mock_mode, i2c_sysfs_device_t *out)
{
    if (!out || bus_nr < 0 || addr < 0x03 || addr > 0x77) return false;
    memset(out, 0, sizeof(*out));
    if (mock_mode) {
        for (size_t i = 0; i < sizeof(s_mock_adapters) / sizeof(s_mock_adapters[0]); i++) {
            const i2c_adapter_info_t *ad = &s_mock_adapters[i];
            if (ad->bus_nr != bus_nr) continue;
            for (size_t d = 0; d < ad->num_devices; d++) {
                if (ad->devices[d].addr == addr) { *out = ad->devices[d]; return true; }
            }
        }
        return false;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/%d-%04x/name", SYSFS_DEVICE_DIR, bus_nr, addr);
    FILE *f = fopen(path, "r");
    if (!f) return false;
    out->addr = addr;
    if (fgets(out->name, sizeof(out->name), f)) out->name[strcspn(out->name, "\n")] = 0;
    fclose(f);
    out->is_bound = i2c_sysfs_is_driver_bound(bus_nr, addr, out->driver, sizeof(out->driver));
    return true;
}

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
static int compare_adapters(const void *a, const void *b)
{
    int x = ((const i2c_adapter_info_t *)a)->bus_nr;
    int y = ((const i2c_adapter_info_t *)b)->bus_nr;
    return (x > y) - (x < y);
}

static int compare_devices(const void *a, const void *b)
{
    return ((const i2c_sysfs_device_t *)a)->addr - ((const i2c_sysfs_device_t *)b)->addr;
}

#endif

int i2c_sysfs_scan_topology(i2c_topology_t *topo, bool mock_mode)
{
    if (!topo) return -EINVAL;
    memset(topo, 0, sizeof(i2c_topology_t));

#if !defined(__linux__) || defined(I2C_FORCE_MOCK)
    if (!mock_mode) return -ENOSYS;
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

        int bus_nr;
        if (i2c_parse_number(entry->d_name + 4, 10, 0, INT_MAX, &bus_nr) < 0) continue;
        i2c_adapter_info_t *ad = &topo->adapters[topo->count];
        ad->bus_nr = bus_nr;
        ad->parent_bus_nr = -1;
        ad->mux_channel = -1;
        ad->is_mux_child = false;

        snprintf(ad->sysfs_path, sizeof(ad->sysfs_path), "%s/i2c-%d", SYSFS_ADAPTER_DIR, bus_nr);

        /* Read adapter name */
        char name_path[512];
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

        /* mux_device points to the immediate MUX, whose channel-N links
         * identify logical buses (including cascaded multiplexers). */
        char mux_link[512], mux_path[PATH_MAX], adapter_path[PATH_MAX];
        snprintf(mux_link, sizeof(mux_link), "%s/mux_device", ad->sysfs_path);
        if (realpath(mux_link, mux_path) && realpath(ad->sysfs_path, adapter_path)) {
            const char *base = strrchr(mux_path, '/');
            unsigned parent, mux_addr;
            char extra;
            if (base && sscanf(base + 1, "%u-%x%c", &parent, &mux_addr, &extra) == 2 && parent <= INT_MAX) {
                ad->is_mux_child = true;
                ad->parent_bus_nr = (int)parent;
                snprintf(ad->parent_device, sizeof(ad->parent_device), "%u-%04x", parent, mux_addr);
                DIR *channels = opendir(mux_path);
                if (channels) {
                    struct dirent *channel;
                    while ((channel = readdir(channels))) {
                        int nr;
                        if (strncmp(channel->d_name, "channel-", 8) ||
                            i2c_parse_number(channel->d_name + 8, 10, 0, INT_MAX, &nr) < 0) continue;
                        char link[PATH_MAX + 300], target[PATH_MAX];
                        snprintf(link, sizeof(link), "%s/%s", mux_path, channel->d_name);
                        if (realpath(link, target) && !strcmp(target, adapter_path)) {
                            ad->mux_channel = nr;
                            break;
                        }
                    }
                    closedir(channels);
                }
            }
        }

        /* Scan devices instantiated on this adapter */

        DIR *dev_dir = opendir(SYSFS_DEVICE_DIR);
        if (dev_dir) {
            struct dirent *dentry;
            while ((dentry = readdir(dev_dir)) != NULL) {
                unsigned device_bus, device_addr;
                char extra;
                if (sscanf(dentry->d_name, "%u-%x%c", &device_bus, &device_addr, &extra) == 2 &&
                    device_bus == (unsigned)bus_nr && device_addr >= 0x03 && device_addr <= 0x77) {
                    if (ad->num_devices >= 32) break;

                    uint8_t d_addr = (uint8_t)device_addr;
                    i2c_sysfs_device_t *dev = &ad->devices[ad->num_devices++];
                    dev->addr = d_addr;

                    char device_name[512];
                    snprintf(device_name, sizeof(device_name), "%s/%d-%04x/name", SYSFS_DEVICE_DIR, bus_nr, d_addr);
                    FILE *nf = fopen(device_name, "r");
                    if (nf) {
                        if (fgets(dev->name, sizeof(dev->name), nf)) dev->name[strcspn(dev->name, "\n")] = 0;
                        fclose(nf);
                    }

                    /* Check driver binding */
                    char drv_link[256];
                    snprintf(drv_link, sizeof(drv_link), "%s/%d-%04x/driver", SYSFS_DEVICE_DIR, bus_nr, d_addr);
                    char drv_target[256];
                    ssize_t link_len = readlink(drv_link, drv_target, sizeof(drv_target) - 1);
                    if (link_len > 0) {
                        drv_target[link_len] = '\0';
                        char *last_slash = strrchr(drv_target, '/');
                        snprintf(dev->driver, sizeof(dev->driver), "%.63s", last_slash ? last_slash + 1 : drv_target);
                        dev->is_bound = true;
                    }
                }
            }
            closedir(dev_dir);
        }

        qsort(ad->devices, ad->num_devices, sizeof(ad->devices[0]), compare_devices);
        topo->count++;
    }

    closedir(dir);
    qsort(topo->adapters, topo->count, sizeof(topo->adapters[0]), compare_adapters);
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
            snprintf(out_driver, driver_len, "%s", slash ? slash + 1 : target);
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
    if (bus_nr < 0 || addr < 0x03 || addr > 0x77 || !chip_name || !*chip_name || strlen(chip_name) >= 20) return -EINVAL;
    for (const char *c = chip_name; *c; c++)
        if (!isalnum((unsigned char)*c) && *c != '-' && *c != '_') return -EINVAL;

    char new_dev_path[256];
    snprintf(new_dev_path, sizeof(new_dev_path), "%s/i2c-%d/new_device", SYSFS_ADAPTER_DIR, bus_nr);

    FILE *f = fopen(new_dev_path, "w");
    if (!f) {
        return -errno;
    }

    int ret = fprintf(f, "%s 0x%02x\n", chip_name, addr);
    int saved = ret < 0 ? errno : 0;
    if (fclose(f) != 0 && !saved) saved = errno;
    return saved ? -saved : 0;
}

int i2c_sysfs_unbind_device(int bus_nr, uint8_t addr)
{
    if (bus_nr < 0 || addr < 0x03 || addr > 0x77) return -EINVAL;
    char del_dev_path[256];
    snprintf(del_dev_path, sizeof(del_dev_path), "%s/i2c-%d/delete_device", SYSFS_ADAPTER_DIR, bus_nr);

    FILE *f = fopen(del_dev_path, "w");
    if (!f) {
        return -errno;
    }

    int ret = fprintf(f, "0x%02x\n", addr);
    int saved = ret < 0 ? errno : 0;
    if (fclose(f) != 0 && !saved) saved = errno;
    return saved ? -saved : 0;
}
