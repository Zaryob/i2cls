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
#include <limits.h>

#include "i2c_types.h"
#include "i2c_sysfs.h"

static void print_bind_help(const char *prog_name)
{
    printf("%sUsage:%s %s <BUS_NUMBER> <DEVICE_ADDRESS> <CHIP_NAME>\n\n", COLOR_BOLD, COLOR_RESET, prog_name);
    printf("Instantiate and bind a Linux kernel driver to an I2C device via sysfs.\n\n");
    printf("%sExample:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  sudo %s 1 0x76 bmp280     # Bind bmp280 kernel driver to 0x76 on i2c-1\n", prog_name);
    printf("  sudo %s 1 0x68 ds1307     # Bind rtc-ds1307 kernel driver to 0x68 on i2c-1\n", prog_name);
}

static void print_unbind_help(const char *prog_name)
{
    printf("%sUsage:%s %s <BUS_NUMBER> <DEVICE_ADDRESS>\n\n", COLOR_BOLD, COLOR_RESET, prog_name);
    printf("Unbind and delete an instantiated I2C device from the Linux kernel.\n\n");
    printf("%sExample:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  sudo %s 1 0x76            # Unbind device at 0x76 on i2c-1\n", prog_name);
}

int cmd_bind_main(int argc, char *argv[])
{
    if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) { print_bind_help(argv[0]); return 0; }
    if (argc != 4) {
        print_bind_help(argv[0]);
        return 1;
    }

    int bus_nr, address;
    if (i2c_parse_number(argv[1], 10, 0, INT_MAX, &bus_nr) < 0 ||
        i2c_parse_number(argv[2], 0, 0x03, 0x77, &address) < 0) {
        fprintf(stderr, "Invalid bus or address (expected 0x03..0x77).\n");
        return 1;
    }
    uint8_t addr = (uint8_t)address;
    const char *chip_name = argv[3];

    printf("Attempting to bind driver '%s%s%s' to address %s0x%02x%s on bus %d...\n",
           COLOR_BOLD, chip_name, COLOR_RESET, COLOR_BOLD, addr, COLOR_RESET, bus_nr);

    int ret = i2c_sysfs_bind_device(bus_nr, addr, chip_name);
    if (ret == 0) {
        printf("%s[+] Successfully instantiated %s at 0x%02x on i2c-%d%s\n",
               COLOR_GREEN, chip_name, addr, bus_nr, COLOR_RESET);
        return 0;
    } else {
        fprintf(stderr, "%s[-] Failed to bind device: %s.%s\n",
                COLOR_RED, strerror(-ret), COLOR_RESET);
        return 1;
    }
}

int cmd_unbind_main(int argc, char *argv[])
{
    if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) { print_unbind_help(argv[0]); return 0; }
    if (argc != 3) {
        print_unbind_help(argv[0]);
        return 1;
    }

    int bus_nr, address;
    if (i2c_parse_number(argv[1], 10, 0, INT_MAX, &bus_nr) < 0 ||
        i2c_parse_number(argv[2], 0, 0x03, 0x77, &address) < 0) {
        fprintf(stderr, "Invalid bus or address (expected 0x03..0x77).\n");
        return 1;
    }
    uint8_t addr = (uint8_t)address;

    printf("Attempting to unbind device at %s0x%02x%s on bus %d...\n",
           COLOR_BOLD, addr, COLOR_RESET, bus_nr);

    int ret = i2c_sysfs_unbind_device(bus_nr, addr);
    if (ret == 0) {
        printf("%s[+] Successfully deleted device at 0x%02x on i2c-%d%s\n",
               COLOR_GREEN, addr, bus_nr, COLOR_RESET);
        return 0;
    } else {
        fprintf(stderr, "%s[-] Failed to unbind device: %s.%s\n",
                COLOR_RED, strerror(-ret), COLOR_RESET);
        return 1;
    }
}
