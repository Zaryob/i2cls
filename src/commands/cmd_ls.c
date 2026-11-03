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
#include <limits.h>

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
    printf("  -i, --identify    Read chip signatures (may change device state)\n");
    printf("      --list-vendors List manufacturer catalog without accessing hardware\n");
    printf("      --list-chips   List chip catalog and signature support\n");
    printf("      --vendor NAME  Filter by manufacturer ID, full name or alias\n");
    printf("  -j, --json        Output results in JSON format for automation\n");
    printf("  -m, --mock        Use simulated mock I2C environment (for testing)\n");
    printf("      --no-color    Disable ANSI color escape sequences\n");
    printf("  -h, --help        Show this help message and exit\n");
    printf("  -v, --version     Print version information\n\n");
    printf("%sExamples:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s 1              # Probe addresses on /dev/i2c-1\n", prog_name);
    printf("  %s -a             # Discover and scan all I2C buses\n", prog_name);
    printf("  %s --mock         # Run in test simulation mode\n", prog_name);
}

static void print_catalog(bool list_vendors, bool json_mode, const i2c_vendor_info_t *filter)
{
    bool first = true;
    if (json_mode) printf("{\"%s\":[", list_vendors ? "vendors" : "chips");
    else if (list_vendors) printf("%-14s %-22s %6s %10s  %s\n", "ID", "VENDOR", "CHIPS", "SIGNATURES", "ALIASES");
    else printf("%-18s %-14s %-18s %-12s %s\n", "CHIP", "VENDOR ID", "KERNEL DEVICE", "SIGNATURE", "ADDRESSES");
    size_t count = list_vendors ? i2c_vendor_count() : i2c_db_get_total_chips();
    for (size_t i = 0; i < count; i++) {
        const i2c_chip_info_t *chip = list_vendors ? NULL : i2c_db_get_chip_by_index(i);
        const i2c_vendor_info_t *vendor = list_vendors ? i2c_vendor_at(i) : i2c_vendor_get(chip->vendor_id);
        if (filter && vendor != filter) continue;
        if (json_mode && !first) putchar(',');
        first = false;
        if (list_vendors) {
            size_t chips = 0, signatures = 0;
            for (size_t c = 0; c < i2c_db_get_total_chips(); c++) {
                const i2c_chip_info_t *entry = i2c_db_get_chip_by_index(c);
                if (entry->vendor_id == vendor->key) {
                    chips++;
                    if (entry->num_rules) signatures++;
                }
            }
            if (json_mode) {
                printf("{\"id\":"); i2c_print_json_string(vendor->id);
                printf(",\"name\":"); i2c_print_json_string(vendor->name);
                printf(",\"chip_count\":%zu,\"signature_chip_count\":%zu,\"aliases\":[", chips, signatures);
                for (size_t a = 0; a < 8 && vendor->aliases[a]; a++) {
                    if (a) putchar(',');
                    i2c_print_json_string(vendor->aliases[a]);
                }
                printf("]}");
            } else {
                printf("%-14s %-22s %6zu %10zu  ", vendor->id, vendor->name, chips, signatures);
                for (size_t a = 0; a < 8 && vendor->aliases[a]; a++) printf("%s%s", a ? ", " : "", vendor->aliases[a]);
                putchar('\n');
            }
        } else {
            if (json_mode) {
                printf("{\"name\":"); i2c_print_json_string(chip->name);
                printf(",\"vendor_id\":"); i2c_print_json_string(vendor->id);
                printf(",\"vendor\":"); i2c_print_json_string(vendor->name);
                printf(",\"category\":"); i2c_print_json_string(i2c_category_to_string(chip->category));
                printf(",\"description\":"); i2c_print_json_string(chip->description);
                printf(",\"kernel_driver\":");
                if (chip->kernel_driver) i2c_print_json_string(chip->kernel_driver); else printf("null");
                printf(",\"kernel_device\":");
                if (chip->kernel_device) i2c_print_json_string(chip->kernel_device); else printf("null");
                printf(",\"signature_supported\":%s,\"addresses\":[", chip->num_rules ? "true" : "false");
            } else {
                printf("%-18s %-14s %-18s %-12s ", chip->name, vendor->id,
                       chip->kernel_device ? chip->kernel_device : "-", chip->num_rules ? "yes" : "metadata");
            }
            for (size_t a = 0; a < sizeof(chip->valid_addrs) && chip->valid_addrs[a]; a++) {
                if (json_mode) printf("%s\"0x%02x\"", a ? "," : "", chip->valid_addrs[a]);
                else printf("%s0x%02x", a ? ", " : "", chip->valid_addrs[a]);
            }
            if (json_mode) printf("]}"); else putchar('\n');
        }
    }
    if (json_mode) printf("]}\n");
}

typedef struct {
    uint8_t addr;
    const i2c_chip_info_t *chip;
    char bound_driver[64];
    char kernel_name[64];
    const i2c_vendor_info_t *vendor;
    const char *vendor_source;
    bool is_bound;
} discovered_dev_t;

static int scan_single_bus(int bus_nr, bool json_mode, bool mock_mode, bool is_first_json, bool identify, const i2c_vendor_info_t *vendor_filter)
{
    i2c_bus_ctx_t *ctx = i2c_bus_open(bus_nr, mock_mode);
    if (!ctx) {
        fprintf(stderr, "Failed to open /dev/i2c-%d: %s\n", bus_nr, strerror(errno));
        return -1;
    }

    discovered_dev_t devs[128] = {0};
    int probe_errors = 0;
    size_t dev_count = 0;

    for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        i2c_sysfs_device_t known = {0};
        i2c_sysfs_get_device(bus_nr, addr, ctx->is_mock, &known);
        bool bound = known.is_bound;

        i2c_probe_status_t st = i2c_bus_probe_addr(ctx, addr);
        if (st == PROBE_ERROR) probe_errors++;
        if (st == PROBE_ACK_UNBOUND || st == PROBE_ACK_BOUND || bound) {
            discovered_dev_t *d = &devs[dev_count++];
            memset(d, 0, sizeof(*d));
            d->addr = addr;
            d->is_bound = bound || (st == PROBE_ACK_BOUND);
            if (bound) {
                snprintf(d->bound_driver, sizeof(d->bound_driver), "%s", known.driver);
            }

            snprintf(d->kernel_name, sizeof(d->kernel_name), "%s", known.name);

            /* Fingerprint device */
            i2c_vendor_id_t filter = vendor_filter ? vendor_filter->key : VENDOR_UNKNOWN;
            d->chip = identify && !d->is_bound ? i2c_db_identify_for_vendor(ctx, addr, filter) : NULL;
            const i2c_chip_info_t *registered = i2c_db_find_kernel_device(known.name, addr);
            if (d->chip) {
                d->vendor = i2c_vendor_get(d->chip->vendor_id);
                d->vendor_source = "signature";
            } else if (registered) {
                d->vendor = i2c_vendor_get(registered->vendor_id);
                d->vendor_source = "kernel";
            }
            if (vendor_filter && d->vendor != vendor_filter) dev_count--;
        }
    }

    if (json_mode) {
        if (!is_first_json) printf(",\n");
        printf("    {\n");
        printf("      \"bus\": %d,\n", bus_nr);
        printf("      \"adapter\": "); i2c_print_json_string(ctx->adapter_name); printf(",\n");
        printf("      \"devices\": [\n");
        for (size_t i = 0; i < dev_count; i++) {
            printf("        {\n");
            printf("          \"address\": \"0x%02x\",\n", devs[i].addr);
            printf("          \"bound\": %s,\n", devs[i].is_bound ? "true" : "false");
            if (devs[i].kernel_name[0]) {
                printf("          \"kernel_name\": "); i2c_print_json_string(devs[i].kernel_name); printf(",\n");
            }
            if (devs[i].is_bound) {
                printf("          \"active_driver\": "); i2c_print_json_string(devs[i].bound_driver); printf(",\n");
            }
            if (devs[i].vendor) {
                printf("          \"vendor\": "); i2c_print_json_string(devs[i].vendor->name); printf(",\n");
                printf("          \"vendor_id\": "); i2c_print_json_string(devs[i].vendor->id); printf(",\n");
                printf("          \"vendor_source\": "); i2c_print_json_string(devs[i].vendor_source); printf(",\n");
            } else {
                printf("          \"vendor\": null, \"vendor_id\": null, \"vendor_source\": null,\n");
            }
            if (devs[i].chip) {
                printf("          \"chip\": \"%s\",\n", devs[i].chip->name);
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
            printf("  %sNo matching devices on bus %d.%s\n", COLOR_GRAY, bus_nr, COLOR_RESET);
        } else {
            printf("  %-6s  %-14s  %-18s  %-24s  %s\n",
                   "ADDR", "CHIP", "VENDOR", "CATEGORY", "STATUS / DRIVER");
            printf("  ----------------------------------------------------------------------------------------\n");

            for (size_t i = 0; i < dev_count; i++) {
                const char *chip_name = devs[i].chip ? devs[i].chip->name : (devs[i].kernel_name[0] ? devs[i].kernel_name : "Unknown");
                const char *vendor = devs[i].vendor ? devs[i].vendor->name : "-";
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
                if (devs[i].vendor_source) printf("           Vendor source: %s\n", devs[i].vendor_source);
            }
        }
    }

    if (probe_errors) fprintf(stderr, "Bus %d: %d address probes failed or are unsupported.\n", bus_nr, probe_errors);
    i2c_bus_close(ctx);
    return probe_errors ? 1 : 0;
}

int cmd_ls_main(int argc, char *argv[])
{
    bool list_vendors = false, list_chips = false;
    const char *vendor_query = NULL;
    bool identify = false;
    bool scan_all = false;
    bool json_mode = false;
    bool mock_mode = false;
    int target_bus = -1;

    static struct option long_options[] = {
        {"list-vendors", no_argument, 0, 1001},
        {"list-chips", no_argument, 0, 1002},
        {"vendor", required_argument, 0, 1003},
        {"identify", no_argument,      0, 'i'},
        {"all",      no_argument,       0, 'a'},
        {"json",     no_argument,       0, 'j'},
        {"mock",     no_argument,       0, 'm'},
        {"no-color", no_argument,       0, 1000},
        {"help",     no_argument,       0, 'h'},
        {"version",  no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "aijmhv", long_options, NULL)) != -1) {
        switch (opt) {
            case 1001: list_vendors = true; break;
            case 1002: list_chips = true; break;
            case 1003: vendor_query = optarg; break;
            case 'i': identify = true; break;
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

    if (argc - optind > 1 || (scan_all && optind < argc) ||
        (optind < argc && i2c_parse_number(argv[optind], 10, 0, INT_MAX, &target_bus) < 0)) {
        fprintf(stderr, "Expected a non-negative bus number, or --all.\n");
        return 1;
    }
    const i2c_vendor_info_t *vendor_filter = NULL;
    if (vendor_query) {
        vendor_filter = i2c_vendor_find(vendor_query);
        if (!vendor_filter) {
            fprintf(stderr, "Unknown vendor '%s'. Use --list-vendors to see accepted names.\n", vendor_query);
            return 1;
        }
    }
    if (list_vendors || list_chips) {
        if ((list_vendors && list_chips) || scan_all || identify || mock_mode || target_bus >= 0) {
            fprintf(stderr, "Catalog modes cannot be combined with each other or hardware scan options.\n");
            return 1;
        }
        print_catalog(list_vendors, json_mode, vendor_filter);
        return 0;
    }
    i2c_topology_t topo;
    if (target_bus < 0) {
        int ret = i2c_sysfs_scan_topology(&topo, mock_mode);
        if (ret < 0) {
            fprintf(stderr, "Cannot discover I2C adapters: %s\n", strerror(-ret));
            return 1;
        }
    }
    int status = 0;
    bool first = true;
    if (json_mode) {
        printf("{\n  \"buses\": [\n");
    }

    if (target_bus < 0) {
        if (topo.count == 0) {
            fprintf(stderr, "No I2C adapters found. Check i2c-dev and device permissions.\n");
            status = 1;
        }
        for (size_t i = 0; i < topo.count; i++) {
            int ret = scan_single_bus(topo.adapters[i].bus_nr, json_mode, mock_mode, first, identify, vendor_filter);
            if (ret >= 0) first = false;
            if (ret) status = 1;
        }
    } else {
        status = scan_single_bus(target_bus, json_mode, mock_mode, true, identify, vendor_filter) != 0;
    }

    if (json_mode) {
        printf("\n  ]\n}\n");
    } else {
        printf("\n");
    }

    return status;
}
