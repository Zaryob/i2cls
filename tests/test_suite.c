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
#include <assert.h>

#include "i2c_types.h"
#include "i2c_bus.h"
#include "i2c_db.h"
#include "i2c_sysfs.h"

static int s_tests_run = 0;
static int s_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    s_tests_run++; \
    if (cond) { \
        s_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
    } \
} while (0)

static void test_categories(void)
{
    printf("\nRunning Test: Categories\n");
    TEST_ASSERT(strcmp(i2c_category_to_string(CAT_IMU), "IMU / Motion") == 0, "CAT_IMU string match");
    TEST_ASSERT(strcmp(i2c_category_to_string(CAT_ENVIRONMENTAL), "Environmental / Weather") == 0, "CAT_ENVIRONMENTAL string match");
    TEST_ASSERT(strcmp(i2c_category_to_string(CAT_DISPLAY), "Display Controller") == 0, "CAT_DISPLAY string match");
    TEST_ASSERT(strcmp(i2c_category_to_string(CAT_RTC), "Real-Time Clock") == 0, "CAT_RTC string match");
}

static void test_chip_database_integrity(void)
{
    printf("\nRunning Test: Chip Database Integrity\n");
    size_t count = i2c_db_get_total_chips();
    TEST_ASSERT(count >= 20, "Database contains at least 20 chips");

    for (size_t i = 0; i < count; i++) {
        const i2c_chip_info_t *chip = i2c_db_get_chip_by_index(i);
        TEST_ASSERT(chip != NULL, "Chip index valid");
        TEST_ASSERT(chip->name != NULL && strlen(chip->name) > 0, "Chip has non-empty name");
        TEST_ASSERT(chip->vendor != NULL && strlen(chip->vendor) > 0, "Chip has non-empty vendor");
        TEST_ASSERT(chip->description != NULL, "Chip has description");
        TEST_ASSERT(chip->num_rules <= 3, "Rule count fits fixed array");
        for (size_t r = 0; r < chip->num_rules; r++) {
            TEST_ASSERT(chip->rules[r].read_len > 0 && chip->rules[r].read_len <= 8 &&
                        chip->rules[r].read_offset < chip->rules[r].read_len,
                        "Rule requires a complete bounded response");
        }
    }
}

static void test_fingerprint_identification(void)
{
    printf("\nRunning Test: Fingerprint Identification Engine\n");
    i2c_bus_ctx_t *ctx = i2c_bus_open(1, true /* mock */);
    TEST_ASSERT(ctx != NULL, "Mock bus opened successfully");

    /* Test BME280 at 0x76 */
    const i2c_chip_info_t *bme = i2c_db_identify_device(ctx, 0x76);
    TEST_ASSERT(bme != NULL, "0x76 identified");
    if (bme) {
        TEST_ASSERT(strcmp(bme->name, "BME280") == 0, "0x76 recognized as BME280");
        TEST_ASSERT(bme->category == CAT_ENVIRONMENTAL, "BME280 category is environmental");
    }

    /* Test MPU-6050 at 0x68 */
    const i2c_chip_info_t *mpu = i2c_db_identify_device(ctx, 0x68);
    TEST_ASSERT(mpu != NULL, "0x68 identified");
    if (mpu) {
        TEST_ASSERT(strcmp(mpu->name, "MPU-6050") == 0, "0x68 recognized as MPU-6050");
        TEST_ASSERT(mpu->category == CAT_IMU, "MPU-6050 category is IMU");
    }

    TEST_ASSERT(i2c_db_identify_device(ctx, 0x3c) == NULL, "OLED address/status cannot establish chip identity");
    TEST_ASSERT(i2c_db_identify_device(ctx, 0x50) == NULL, "EEPROM address does not imply model");
    TEST_ASSERT(i2c_db_identify_device(ctx, 0x77) == NULL, "Absent device does not become a TCA9548A");
    TEST_ASSERT(i2c_bus_probe_addr(ctx, 0x12) == PROBE_NO_ACK, "Absent device has no ACK");
    TEST_ASSERT(i2c_bus_probe_addr(ctx, 0x76) == PROBE_ACK_UNBOUND, "Mock BME280 responds");
    TEST_ASSERT(i2c_bus_probe_addr(ctx, 0x80) == PROBE_ERROR, "Invalid address rejected");
    TEST_ASSERT(i2c_bus_open(-1, true) == NULL, "Negative bus rejected");

    /* Test unmapped random address */
    const i2c_chip_info_t *unknown = i2c_db_identify_device(ctx, 0x12);
    TEST_ASSERT(unknown == NULL, "0x12 correctly returns NULL (unknown)");

    i2c_bus_close(ctx);
}

static void test_sysfs_topology(void)
{
    printf("\nRunning Test: Sysfs Topology Scanner\n");
    i2c_topology_t topo;
    int ret = i2c_sysfs_scan_topology(&topo, true /* mock */);
    TEST_ASSERT(ret == 0, "Topology scan succeeded");
    TEST_ASSERT(topo.count == 3, "Mock topology contains 3 adapters (1 root + 2 mux channels)");
    TEST_ASSERT(topo.adapters[0].bus_nr == 1, "Root adapter is bus 1");
    TEST_ASSERT(topo.adapters[1].is_mux_child == true, "Adapter 2 is MUX child");
    TEST_ASSERT(topo.adapters[1].parent_bus_nr == 1, "Adapter 2 parent is bus 1");
    TEST_ASSERT(topo.adapters[1].mux_channel == 0, "Adapter 2 is mux channel 0");
    TEST_ASSERT(topo.adapters[2].mux_channel == 1, "Adapter 3 is mux channel 1");
}

int main(void)
{
    g_enable_color = false;
    i2c_db_init();

    printf("===========================================\n");
    printf("   i2c-tools-ng Automated Test Suite       \n");
    printf("===========================================\n");

    int number;
    TEST_ASSERT(i2c_parse_number("256", 0, 3, 0x77, &number) < 0, "Address cannot wrap around");
    TEST_ASSERT(i2c_parse_number("garbage", 10, 0, 1000, &number) < 0, "Bus must be numeric");
    TEST_ASSERT(i2c_parse_number("1tail", 10, 0, 1000, &number) < 0, "Trailing garbage rejected");
    TEST_ASSERT(i2c_parse_number("-1", 10, 0, 1000, &number) < 0, "Negative input rejected");
    TEST_ASSERT(i2c_parse_number("0x76", 0, 3, 0x77, &number) == 0 && number == 0x76, "Hex address parsed");
    test_categories();
    test_chip_database_integrity();
    test_fingerprint_identification();
    test_sysfs_topology();

    printf("\n-------------------------------------------\n");
    printf("Results: %d / %d tests passed.\n", s_tests_passed, s_tests_run);
    printf("===========================================\n");

    return (s_tests_passed == s_tests_run) ? 0 : 1;
}
