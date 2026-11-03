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
#include <limits.h>
#include <errno.h>

#include "i2c_types.h"
#include "i2c_bus.h"
#include "i2c_db.h"
#include "i2c_sysfs.h"

static void print_help(const char *prog_name)
{
    printf("%sUsage:%s %s [OPTIONS] <BUS_NUMBER> <DEVICE_ADDRESS>\n\n", COLOR_BOLD, COLOR_RESET, prog_name);
    printf("Perform in-depth interrogation and diagnostic inspection on an I2C address.\n\n");
    printf("%sOptions:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  -m, --mock        Run in mock simulation mode\n");
    printf("      --no-color    Disable ANSI colors\n");
    printf("  -h, --help        Show this help message\n\n");
    printf("%sExample:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s 1 0x76         # Probe address 0x76 on bus 1\n", prog_name);
    printf("  %s -m 1 0x68      # Test probe 0x68 in mock mode\n", prog_name);
}

int cmd_probe_main(int argc, char *argv[])
{
    bool mock_mode = false;

    static struct option long_options[] = {
        {"mock",     no_argument, 0, 'm'},
        {"no-color", no_argument, 0, 1000},
        {"help",     no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "mh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm': mock_mode = true; break;
            case 1000: g_enable_color = false; break;
            case 'h':
                print_help(argv[0]);
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }

    if (argc - optind != 2) {
        print_help(argv[0]);
        return 1;
    }

    int bus_nr, address;
    if (i2c_parse_number(argv[optind], 10, 0, INT_MAX, &bus_nr) < 0 ||
        i2c_parse_number(argv[optind + 1], 0, 0x03, 0x77, &address) < 0) {
        fprintf(stderr, "Invalid bus or address (expected 0x03..0x77).\n");
        return 1;
    }
    uint8_t addr = (uint8_t)address;

    if (!isatty(STDOUT_FILENO)) {
        g_enable_color = false;
    }

    printf("\n%s=== Deep I2C Device Probe: Bus %d, Address 0x%02x ===%s\n\n",
           COLOR_BOLD, bus_nr, addr, COLOR_RESET);

    i2c_bus_ctx_t *ctx = i2c_bus_open(bus_nr, mock_mode);
    if (!ctx) {
        fprintf(stderr, "%s[-] Failed to open bus %d%s\n", COLOR_RED, bus_nr, COLOR_RESET);
        return 1;
    }

    /* 1. Adapter Capabilities */
    printf("%s[1] Adapter Information:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("    Path            : %s\n", ctx->path);
    printf("    Name            : %s\n", ctx->adapter_name);
    printf("    Mode            : %s\n", ctx->is_mock ? "Mock/Simulation" : "Hardware Kernel Access");
    i2c_bus_print_funcs(ctx);
    printf("\n");

    /* 2. Sysfs Driver Binding */
    i2c_sysfs_device_t known = {0};
    i2c_sysfs_get_device(bus_nr, addr, ctx->is_mock, &known);
    bool is_bound = known.is_bound;
    const i2c_chip_info_t *registered = i2c_db_find_kernel_device(known.name, addr);
    printf("%s[2] Linux Kernel Driver Status:%s\n", COLOR_BOLD, COLOR_RESET);
    if (is_bound) {
        printf("    Status          : %sBound%s\n", COLOR_YELLOW, COLOR_RESET);
        printf("    Active Driver   : %s%s%s\n", COLOR_BOLD, known.driver, COLOR_RESET);
    } else {
        printf("    Status          : %sUnbound / Available%s (No kernel driver claimed address)\n",
               COLOR_GREEN, COLOR_RESET);
    }
    if (registered) {
        const i2c_vendor_info_t *vendor = i2c_vendor_get(registered->vendor_id);
        printf("    Kernel Device   : %s\n", known.name);
        printf("    Manufacturer    : %s (%s)\n", vendor->name, vendor->id);
        printf("    Vendor Source   : kernel configuration\n");
    }
    printf("\n");

    /* 3. Hardware Probing */
    printf("%s[3] Bus ACK Probe:%s\n", COLOR_BOLD, COLOR_RESET);
    i2c_probe_status_t st = i2c_bus_probe_addr(ctx, addr);
    if (st == PROBE_NO_ACK) {
        printf("    Result          : %sDevice did not ACK at 0x%02x%s\n\n", COLOR_RED, addr, COLOR_RESET);
        i2c_bus_close(ctx);
        return 1;
    }
    if (st == PROBE_ERROR) {
        fprintf(stderr, "Probe failed or unsupported: %s\n", strerror(errno));
        i2c_bus_close(ctx);
        return 1;
    }
    if (st == PROBE_ACK_BOUND) {
        printf("    Address claimed by kernel; raw probing skipped.\n");
        i2c_bus_close(ctx);
        return 0;
    }
    printf("    Result          : %sACK Received! Device is responding on bus.%s\n\n", COLOR_GREEN, COLOR_RESET);

    /* 4. Fingerprint Identification */
    printf("%s[4] Device Fingerprint Identification:%s\n", COLOR_BOLD, COLOR_RESET);
    const i2c_chip_info_t *chip = i2c_db_identify_device(ctx, addr);
    if (chip) {
        printf("    Detected Chip   : %s%s%s\n", COLOR_CYAN, chip->name, COLOR_RESET);
        printf("    Manufacturer    : %s\n", i2c_vendor_get(chip->vendor_id)->name);
        printf("    Vendor Source   : signature\n");
        printf("    Category        : %s\n", i2c_category_to_string(chip->category));
        printf("    Description     : %s\n", chip->description);
        if (chip->kernel_driver) {
            printf("    Kernel Driver   : %s%s%s\n", COLOR_GREEN, chip->kernel_driver, COLOR_RESET);

        }
    } else {
        printf("    Detected Chip   : %sUnknown or Generic I2C Peripheral%s\n", COLOR_YELLOW, COLOR_RESET);
    }
    printf("\n");

    /* Avoid arbitrary register writes on devices with unknown protocols. */
    if (!chip) printf("No verified signature; arbitrary register dump skipped.\n");

    i2c_bus_close(ctx);
    return 0;
}
