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
#include <getopt.h>

#include "i2c_types.h"
#include "i2c_bus.h"
#include "i2c_db.h"
#include "i2c_sysfs.h"

static void print_help(const char *prog_name)
{
    printf("%sUsage:%s %s [OPTIONS] [BUS_NUMBER]\n\n", COLOR_BOLD, COLOR_RESET, prog_name);
    printf("Smart I2C bus scanner and automatic chip identification tool.\n\n");
    printf("%sOptions:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  -a, --all         Scan all available I2C buses sequentially\n");
    printf("  -j, --json        Output results in JSON format for automation\n");
    printf("  -m, --mock        Use simulated mock I2C environment (for testing)\n");
    printf("      --no-color    Disable ANSI color escape sequences\n");
    printf("  -h, --help        Show this help message and exit\n");
    printf("  -v, --version     Print version information\n\n");
    printf("%sExamples:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s 1              # Scan /dev/i2c-1 with smart chip identification\n", prog_name);
    printf("  %s -a             # Discover and scan all I2C buses\n", prog_name);
    printf("  %s --mock         # Run in test simulation mode\n", prog_name);
}

typedef struct {
    uint8_t addr;
    const i2c_chip_info_t *chip;
    char bound_driver[64];
    bool is_bound;
} discovered_dev_t;

static int scan_single_bus(int bus_nr, bool json_mode, bool mock_mode, bool is_first_json)
{
    i2c_bus_ctx_t *ctx = i2c_bus_open(bus_nr, mock_mode);
    if (!ctx) {
        if (!json_mode) {
            fprintf(stderr, "%s[-] Failed to open I2C bus /dev/i2c-%d%s\n", COLOR_RED, bus_nr, COLOR_RESET);
        }
        return -1;
    }

    discovered_dev_t devs[128];
    size_t dev_count = 0;

    for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        char driver[64] = {0};
        bool bound = i2c_sysfs_is_driver_bound(bus_nr, addr, driver, sizeof(driver));

        i2c_probe_status_t st = i2c_bus_probe_addr(ctx, addr);
        if (st == PROBE_ACK_UNBOUND || st == PROBE_ACK_BOUND || bound) {
            discovered_dev_t *d = &devs[dev_count++];
            d->addr = addr;
            d->is_bound = bound || (st == PROBE_ACK_BOUND);
            if (bound) {
                strncpy(d->bound_driver, driver, sizeof(d->bound_driver) - 1);
            }

            /* Fingerprint device */
            d->chip = i2c_db_identify_device(ctx, addr);
        }
    }

    if (json_mode) {
        if (!is_first_json) printf(",\n");
        printf("    {\n");
        printf("      \"bus\": %d,\n", bus_nr);
        printf("      \"adapter\": \"%s\",\n", ctx->adapter_name);
        printf("      \"devices\": [\n");
        for (size_t i = 0; i < dev_count; i++) {
            printf("        {\n");
            printf("          \"address\": \"0x%02x\",\n", devs[i].addr);
            printf("          \"bound\": %s,\n", devs[i].is_bound ? "true" : "false");
            if (devs[i].is_bound) {
                printf("          \"active_driver\": \"%s\",\n", devs[i].bound_driver);
            }
            if (devs[i].chip) {
                printf("          \"chip\": \"%s\",\n", devs[i].chip->name);
                printf("          \"vendor\": \"%s\",\n", devs[i].chip->vendor);
                printf("          \"category\": \"%s\",\n", i2c_category_to_string(devs[i].chip->category));
                printf("          \"description\": \"%s\",\n", devs[i].chip->description);
                printf("          \"kernel_driver\": \"%s\"\n", devs[i].chip->kernel_driver ? devs[i].chip->kernel_driver : "none");
            } else {
                printf("          \"chip\": \"Unknown\",\n");
                printf("          \"category\": \"General I2C Device\"\n");
            }
            printf("        }%s\n", (i + 1 < dev_count) ? "," : "");
        }
        printf("      ]\n");
        printf("    }");
    } else {
        printf("\n%s=== I2C Bus %d: %s (%s) ===%s\n",
               COLOR_BOLD, bus_nr, ctx->adapter_name, ctx->path, COLOR_RESET);

        if (dev_count == 0) {
            printf("  %sNo devices responded on bus %d.%s\n", COLOR_GRAY, bus_nr, COLOR_RESET);
        } else {
            printf("  %-6s  %-14s  %-18s  %-24s  %s\n",
                   "ADDR", "CHIP", "VENDOR", "CATEGORY", "STATUS / DRIVER");
            printf("  ----------------------------------------------------------------------------------------\n");

            for (size_t i = 0; i < dev_count; i++) {
                const char *chip_name = devs[i].chip ? devs[i].chip->name : "Unknown";
                const char *vendor = devs[i].chip ? devs[i].chip->vendor : "-";
                const char *cat = devs[i].chip ? i2c_category_to_string(devs[i].chip->category) : "General";

                char status[128];
                if (devs[i].is_bound) {
                    snprintf(status, sizeof(status), "%s[Bound: %s]%s",
                             COLOR_YELLOW,
                             devs[i].bound_driver[0] ? devs[i].bound_driver : "kernel",
                             COLOR_RESET);
                } else if (devs[i].chip && devs[i].chip->kernel_driver) {
                    snprintf(status, sizeof(status), "%s[Driver: %s]%s",
                             COLOR_GREEN, devs[i].chip->kernel_driver, COLOR_RESET);
                } else {
                    snprintf(status, sizeof(status), "%s[Free / Unbound]%s", COLOR_BLUE, COLOR_RESET);
                }

                printf("  %s0x%02x%s   %-14s  %-18s  %-24s  %s\n",
                       COLOR_BOLD, devs[i].addr, COLOR_RESET,
                       chip_name, vendor, cat, status);
            }
        }
    }

    i2c_bus_close(ctx);
    return 0;
}

int cmd_ls_main(int argc, char *argv[])
{
    bool scan_all = false;
    bool json_mode = false;
    bool mock_mode = false;
    int target_bus = -1;

    static struct option long_options[] = {
        {"all",      no_argument,       0, 'a'},
        {"json",     no_argument,       0, 'j'},
        {"mock",     no_argument,       0, 'm'},
        {"no-color", no_argument,       0, 1000},
        {"help",     no_argument,       0, 'h'},
        {"version",  no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "ajmhv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'a':
                scan_all = true;
                break;
            case 'j':
                json_mode = true;
                break;
            case 'm':
                mock_mode = true;
                break;
            case 1000:
                g_enable_color = false;
                break;
            case 'v':
                printf("i2cls version %s (i2c-tools-ng)\n", I2CTOOL_VERSION);
                return 0;
            case 'h':
            default:
                print_help(argv[0]);
                return 0;
        }
    }

    if (!isatty(STDOUT_FILENO)) {
        g_enable_color = false;
    }

    if (optind < argc) {
        target_bus = atoi(argv[optind]);
    }

    i2c_topology_t topo;
    i2c_sysfs_scan_topology(&topo, mock_mode);

    if (json_mode) {
        printf("{\n  \"buses\": [\n");
    }

    if (scan_all || target_bus < 0) {
        if (topo.count == 0) {
            /* Try default bus 1 */
            scan_single_bus(1, json_mode, mock_mode, true);
        } else {
            for (size_t i = 0; i < topo.count; i++) {
                scan_single_bus(topo.adapters[i].bus_nr, json_mode, mock_mode, (i == 0));
            }
        }
    } else {
        scan_single_bus(target_bus, json_mode, mock_mode, true);
    }

    if (json_mode) {
        printf("\n  ]\n}\n");
    } else {
        printf("\n");
    }

    return 0;
}
