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
#include <errno.h>

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

static void print_device_node(const i2c_sysfs_device_t *dev, const char *prefix, bool last)
{
    printf("%s%s── 0x%02x: %s", prefix, last ? "└" : "├", dev->addr,
           dev->name[0] ? dev->name : "I2C device");
    if (dev->is_bound) printf(" [Bound: %s]", dev->driver);
    putchar('\n');
}

static bool has_parent(const i2c_topology_t *topo, const i2c_adapter_info_t *ad)
{
    for (size_t i = 0; i < topo->count; i++)
        if (topo->adapters[i].bus_nr == ad->parent_bus_nr) return true;
    return false;
}

static void print_adapter(const i2c_topology_t *topo, size_t index, const char *prefix,
                          bool last, bool visited[MAX_I2C_BUSES])
{
    if (visited[index]) return;
    visited[index] = true;
    const i2c_adapter_info_t *ad = &topo->adapters[index];
    if (!*prefix) printf("● ");
    else printf("%s%s── [MUX Channel %d] -> ", prefix, last ? "└" : "├", ad->mux_channel);
    printf("i2c-%d: %s (%s)\n", ad->bus_nr, ad->name, ad->sysfs_path);

    size_t remaining = ad->num_devices;
    for (size_t i = 0; i < topo->count; i++)
        if (!visited[i] && topo->adapters[i].is_mux_child && topo->adapters[i].parent_bus_nr == ad->bus_nr) remaining++;
    char child_prefix[1024];
    snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, !*prefix ? "  " : (last ? "    " : "│   "));
    for (size_t d = 0; d < ad->num_devices; d++)
        print_device_node(&ad->devices[d], child_prefix, --remaining == 0);
    for (size_t i = 0; i < topo->count; i++)
        if (!visited[i] && topo->adapters[i].is_mux_child && topo->adapters[i].parent_bus_nr == ad->bus_nr)
            print_adapter(topo, i, child_prefix, --remaining == 0, visited);
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
                print_help(argv[0]);
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }

    if (!isatty(STDOUT_FILENO)) {
        g_enable_color = false;
    }

    if (optind != argc) { print_help(argv[0]); return 1; }
    i2c_topology_t topo;
    int ret = i2c_sysfs_scan_topology(&topo, mock_mode);
    if (ret < 0) {
        fprintf(stderr, "Cannot discover I2C adapters: %s\n", strerror(-ret));
        return 1;
    }
    if (!topo.count && !json_mode) {
        fprintf(stderr, "No I2C adapters detected.\n");
        return 1;
    }

    if (json_mode) {
        printf("{\n  \"topology\": [\n");
        for (size_t i = 0; i < topo.count; i++) {
            i2c_adapter_info_t *ad = &topo.adapters[i];
            printf("    {\n");
            printf("      \"bus\": %d,\n", ad->bus_nr);
            printf("      \"name\": "); i2c_print_json_string(ad->name); printf(",\n");
            printf("      \"is_mux_child\": %s,\n", ad->is_mux_child ? "true" : "false");
            if (ad->is_mux_child) {
                printf("      \"parent_bus\": %d,\n", ad->parent_bus_nr);
                printf("      \"mux_channel\": %d,\n", ad->mux_channel);
            }
            printf("      \"devices\": [\n");
            for (size_t d = 0; d < ad->num_devices; d++) {
                const i2c_sysfs_device_t *dev = &ad->devices[d];
                printf("        {\"addr\": \"0x%02x\", \"bound\": %s, \"name\": ",
                       dev->addr, dev->is_bound ? "true" : "false");
                i2c_print_json_string(dev->name);
                printf(", \"driver\": "); i2c_print_json_string(dev->driver);
                printf("}%s\n", d + 1 < ad->num_devices ? "," : "");
            }
            printf("      ]\n");
            printf("    }%s\n", (i + 1 < topo.count) ? "," : "");
        }
        printf("  ]\n}\n");
        return 0;
    }

    printf("\n%sLinux I2C Bus Topology Tree%s\n", COLOR_BOLD, COLOR_RESET);
    printf("=============================\n");

    bool visited[MAX_I2C_BUSES] = {false};
    for (size_t i = 0; i < topo.count; i++) {
        if (!topo.adapters[i].is_mux_child || !has_parent(&topo, &topo.adapters[i]))
            print_adapter(&topo, i, "", true, visited);
    }
    /* Retain visibility even for incomplete or cyclic sysfs snapshots. */
    for (size_t i = 0; i < topo.count; i++)
        if (!visited[i]) print_adapter(&topo, i, "", true, visited);
    putchar('\n');
    return 0;
}
