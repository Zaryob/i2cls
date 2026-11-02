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
#include "i2c_sysfs.h"
#include "i2c_bus.h"
#include "i2c_db.h"

static void print_help(const char *prog_name)
{
    printf("%sUsage:%s %s [OPTIONS]\n\n", COLOR_BOLD, COLOR_RESET, prog_name);
    printf("Visualize Linux I2C bus hierarchy, multiplexers, and attached devices in a tree.\n\n");
    printf("%sOptions:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  -m, --mock        Use simulated mock I2C environment\n");
    printf("  -j, --json        Output topology tree in JSON format\n");
    printf("      --no-color    Disable ANSI colors\n");
    printf("  -h, --help        Show this help message and exit\n");
    printf("  -v, --version     Print version\n");
}

static void print_device_node(int bus_nr, uint8_t addr, bool mock_mode, const char *prefix, bool is_last)
{
    i2c_bus_ctx_t *ctx = i2c_bus_open(bus_nr, mock_mode);
    const i2c_chip_info_t *chip = NULL;
    if (ctx) {
        chip = i2c_db_identify_device(ctx, addr);
        i2c_bus_close(ctx);
    }

    char driver[64] = {0};
    bool bound = i2c_sysfs_is_driver_bound(bus_nr, addr, driver, sizeof(driver));

    const char *chip_name = chip ? chip->name : "I2C Device";
    const char *cat_str = chip ? i2c_category_to_string(chip->category) : "General";

    printf("%s%s── %s0x%02x%s: %s%s%s (%s)",
           prefix,
           is_last ? "└" : "├",
           COLOR_BOLD, addr, COLOR_RESET,
           COLOR_CYAN, chip_name, COLOR_RESET,
           cat_str);

    if (bound) {
        printf(" %s[Bound: %s]%s\n", COLOR_YELLOW, driver[0] ? driver : "kernel", COLOR_RESET);
    } else if (chip && chip->kernel_driver) {
        printf(" %s[Driver: %s]%s\n", COLOR_GREEN, chip->kernel_driver, COLOR_RESET);
    } else {
        printf("\n");
    }
}

int cmd_tree_main(int argc, char *argv[])
{
    bool mock_mode = false;
    bool json_mode = false;

    static struct option long_options[] = {
        {"mock",     no_argument, 0, 'm'},
        {"json",     no_argument, 0, 'j'},
        {"no-color", no_argument, 0, 1000},
        {"help",     no_argument, 0, 'h'},
        {"version",  no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "mjhv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm': mock_mode = true; break;
            case 'j': json_mode = true; break;
            case 1000: g_enable_color = false; break;
            case 'v':
                printf("i2ctree version %s (i2c-tools-ng)\n", I2CTOOL_VERSION);
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

    i2c_topology_t topo;
    i2c_sysfs_scan_topology(&topo, mock_mode);

    if (topo.count == 0) {
        printf("%sNo I2C adapters detected.%s\n", COLOR_RED, COLOR_RESET);
        return 0;
    }

    if (json_mode) {
        printf("{\n  \"topology\": [\n");
        for (size_t i = 0; i < topo.count; i++) {
            i2c_adapter_info_t *ad = &topo.adapters[i];
            printf("    {\n");
            printf("      \"bus\": %d,\n", ad->bus_nr);
            printf("      \"name\": \"%s\",\n", ad->name);
            printf("      \"is_mux_child\": %s,\n", ad->is_mux_child ? "true" : "false");
            if (ad->is_mux_child) {
                printf("      \"parent_bus\": %d,\n", ad->parent_bus_nr);
                printf("      \"mux_channel\": %d,\n", ad->mux_channel);
            }
            printf("      \"devices\": [\n");
            for (size_t d = 0; d < ad->num_devices; d++) {
                printf("        {\"addr\": \"0x%02x\", \"bound\": %s}%s\n",
                       ad->devices[d].addr,
                       ad->devices[d].is_bound ? "true" : "false",
                       (d + 1 < ad->num_devices) ? "," : "");
            }
            printf("      ]\n");
            printf("    }%s\n", (i + 1 < topo.count) ? "," : "");
        }
        printf("  ]\n}\n");
        return 0;
    }

    printf("\n%sLinux I2C Bus Topology Tree%s\n", COLOR_BOLD, COLOR_RESET);
    printf("=============================\n");

    /* Render root adapters first */
    for (size_t i = 0; i < topo.count; i++) {
        i2c_adapter_info_t *ad = &topo.adapters[i];
        if (ad->is_mux_child) continue; /* Will be rendered under parent */

        printf("%s● i2c-%d: %s%s (%s)\n",
               COLOR_MAGENTA, ad->bus_nr, COLOR_RESET, ad->name, ad->sysfs_path);

        /* Print devices directly connected to root */
        for (size_t d = 0; d < ad->num_devices; d++) {
            bool is_last_dev = (d + 1 == ad->num_devices);
            print_device_node(ad->bus_nr, ad->devices[d].addr, mock_mode, "  ", is_last_dev);
        }

        /* Check for child mux channels belonging to this adapter */
        for (size_t c = 0; c < topo.count; c++) {
            i2c_adapter_info_t *child = &topo.adapters[c];
            if (child->is_mux_child && child->parent_bus_nr == ad->bus_nr) {
                printf("  %s└── [MUX Channel %d] -> i2c-%d: %s%s\n",
                       COLOR_BLUE, child->mux_channel, child->bus_nr, child->name, COLOR_RESET);

                for (size_t cd = 0; cd < child->num_devices; cd++) {
                    bool is_last_child_dev = (cd + 1 == child->num_devices);
                    print_device_node(child->bus_nr, child->devices[cd].addr, mock_mode, "      ", is_last_child_dev);
                }
            }
        }
        printf("\n");
    }

    return 0;
}
