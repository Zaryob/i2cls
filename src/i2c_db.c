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

#include <string.h>
#include "i2c_db.h"
#include "i2c_bus.h"

/* Declarative database of supported chips and their identification rules */
static const i2c_chip_info_t s_chip_database[] = {
    /* Bosch Sensortec Environmental */
    {
        .name = "BME280",
        .vendor = "Bosch Sensortec",
        .category = CAT_ENVIRONMENTAL,
        .description = "Combined Temperature, Humidity & Barometric Pressure Sensor",
        .kernel_driver = "bmp280",
        .valid_addrs = {0x76, 0x77, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0xD0}, .cmd_len = 1, .read_len = 1, .expected_val = 0x60, .mask = 0xFF }
        }
    },
    {
        .name = "BMP280",
        .vendor = "Bosch Sensortec",
        .category = CAT_ENVIRONMENTAL,
        .description = "Digital Barometric Pressure & Temperature Sensor",
        .kernel_driver = "bmp280",
        .valid_addrs = {0x76, 0x77, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0xD0}, .cmd_len = 1, .read_len = 1, .expected_val = 0x58, .mask = 0xFF }
        }
    },
    {
        .name = "BMP388",
        .vendor = "Bosch Sensortec",
        .category = CAT_ENVIRONMENTAL,
        .description = "High-Accuracy Barometric Pressure & Temperature Sensor",
        .kernel_driver = "bmp380",
        .valid_addrs = {0x76, 0x77, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 1, .expected_val = 0x50, .mask = 0xFF }
        }
    },
    {
        .name = "BMP180",
        .vendor = "Bosch Sensortec",
        .category = CAT_ENVIRONMENTAL,
        .description = "Digital Barometric Pressure Sensor",
        .kernel_driver = "bmp280",
        .valid_addrs = {0x77, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0xD0}, .cmd_len = 1, .read_len = 1, .expected_val = 0x55, .mask = 0xFF }
        }
    },
    {
        .name = "BME680",
        .vendor = "Bosch Sensortec",
        .category = CAT_ENVIRONMENTAL,
        .description = "Gas, Humidity, Pressure & Temperature Sensor (VOC/MOX)",
        .kernel_driver = "bme680",
        .valid_addrs = {0x76, 0x77, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0xD0}, .cmd_len = 1, .read_len = 1, .expected_val = 0x61, .mask = 0xFF }
        }
    },
    {
        .name = "BNO055",
        .vendor = "Bosch Sensortec",
        .category = CAT_IMU,
        .description = "Intelligent 9-Axis Absolute Orientation Sensor (Fusion IMU)",
        .kernel_driver = "bno055",
        .valid_addrs = {0x28, 0x29, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 1, .expected_val = 0xA0, .mask = 0xFF }
        }
    },

    /* InvenSense / TDK Motion */
    {
        .name = "MPU-6050",
        .vendor = "TDK InvenSense",
        .category = CAT_IMU,
        .description = "6-Axis MotionTracking (3-Axis Gyroscope + 3-Axis Accelerometer)",
        .kernel_driver = "inv-mpu6050",
        .valid_addrs = {0x68, 0x69, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x75}, .cmd_len = 1, .read_len = 1, .expected_val = 0x68, .mask = 0x7E }
        }
    },
    {
        .name = "MPU-6500",
        .vendor = "TDK InvenSense",
        .category = CAT_IMU,
        .description = "6-Axis Gyro + Accelerometer with Low Power Features",
        .kernel_driver = "inv-mpu6050",
        .valid_addrs = {0x68, 0x69, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x75}, .cmd_len = 1, .read_len = 1, .expected_val = 0x70, .mask = 0xFF }
        }
    },
    {
        .name = "MPU-9250",
        .vendor = "TDK InvenSense",
        .category = CAT_IMU,
        .description = "9-Axis Multi-Chip Module MotionTracking Device",
        .kernel_driver = "inv-mpu6050",
        .valid_addrs = {0x68, 0x69, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x75}, .cmd_len = 1, .read_len = 1, .expected_val = 0x71, .mask = 0xFE }
        }
    },
    {
        .name = "MPU-6886",
        .vendor = "TDK InvenSense",
        .category = CAT_IMU,
        .description = "6-Axis Motion Sensor (Common on M5Stack devices)",
        .kernel_driver = "inv-mpu6050",
        .valid_addrs = {0x68, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x75}, .cmd_len = 1, .read_len = 1, .expected_val = 0x19, .mask = 0xFF }
        }
    },
    {
        .name = "ICM-20948",
        .vendor = "TDK InvenSense",
        .category = CAT_IMU,
        .description = "Low-Power 9-Axis MotionTracking Device with DMP",
        .kernel_driver = "inv-icm20948",
        .valid_addrs = {0x68, 0x69, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 1, .expected_val = 0xEA, .mask = 0xFF }
        }
    },

    /* STMicroelectronics */
    {
        .name = "LIS3DH",
        .vendor = "STMicroelectronics",
        .category = CAT_IMU,
        .description = "Ultra-Low-Power High-Performance 3-Axis Linear Accelerometer",
        .kernel_driver = "st_accel",
        .valid_addrs = {0x18, 0x19, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x0F}, .cmd_len = 1, .read_len = 1, .expected_val = 0x33, .mask = 0xFF }
        }
    },
    {
        .name = "LSM6DS3",
        .vendor = "STMicroelectronics",
        .category = CAT_IMU,
        .description = "Always-on 3D Accelerometer and 3D Gyroscope with Embedded FIFO",
        .kernel_driver = "st_lsm6dsx",
        .valid_addrs = {0x6A, 0x6B, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x0F}, .cmd_len = 1, .read_len = 1, .expected_val = 0x69, .mask = 0xFF }
        }
    },
    {
        .name = "LPS25H",
        .vendor = "STMicroelectronics",
        .category = CAT_ENVIRONMENTAL,
        .description = "MEMS Piezoresistive Absolute Barometer & Altimeter",
        .kernel_driver = "st_pressure",
        .valid_addrs = {0x5C, 0x5D, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x0F}, .cmd_len = 1, .read_len = 1, .expected_val = 0xBD, .mask = 0xFF }
        }
    },
    {
        .name = "HTS221",
        .vendor = "STMicroelectronics",
        .category = CAT_ENVIRONMENTAL,
        .description = "Capacitive Digital Relative Humidity and Temperature Sensor",
        .kernel_driver = "hts221",
        .valid_addrs = {0x5F, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x0F}, .cmd_len = 1, .read_len = 1, .expected_val = 0xBC, .mask = 0xFF }
        }
    },

    /* Analog Devices */
    {
        .name = "ADXL345",
        .vendor = "Analog Devices",
        .category = CAT_IMU,
        .description = "3-Axis Digital Accelerometer with +/- 16g Measurement Range",
        .kernel_driver = "adxl345",
        .valid_addrs = {0x53, 0x1D, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 1, .expected_val = 0xE5, .mask = 0xFF }
        }
    },

    /* Maxim Integrated / Analog Devices RTC */
    {
        .name = "DS3231",
        .vendor = "Maxim Integrated",
        .category = CAT_RTC,
        .description = "Extremely Accurate I2C Integrated RTC/TCXO/Crystal",
        .kernel_driver = "rtc-ds1307",
        .valid_addrs = {0x68, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x0E}, .cmd_len = 1, .read_len = 1, .expected_val = 0x1C, .mask = 0x1C }
        }
    },
    {
        .name = "DS1307",
        .vendor = "Maxim Integrated",
        .category = CAT_RTC,
        .description = "64 x 8 Serial Real-Time Clock with 56-Byte NV RAM",
        .kernel_driver = "rtc-ds1307",
        .valid_addrs = {0x68, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x07}, .cmd_len = 1, .read_len = 1, .expected_val = 0x03, .mask = 0x13 }
        }
    },
    {
        .name = "MAX44009",
        .vendor = "Maxim Integrated",
        .category = CAT_OPTICAL_TOF,
        .description = "Industry's Lowest-Power Ambient Light Sensor with ADC",
        .kernel_driver = "max44009",
        .valid_addrs = {0x4A, 0x4B, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 1, .expected_val = 0x00, .mask = 0xFE }
        }
    },

    /* Texas Instruments Power & Conversion */
    {
        .name = "INA219",
        .vendor = "Texas Instruments",
        .category = CAT_POWER_ADC,
        .description = "High-Side, Bidirectional Zero-Drift Current/Power Monitor",
        .kernel_driver = "ina219",
        .valid_addrs = {0x40, 0x41, 0x44, 0x45, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 2, .expected_val = 0x39, .mask = 0xFF, .read_offset = 0 }
        }
    },
    {
        .name = "ADS1115",
        .vendor = "Texas Instruments",
        .category = CAT_POWER_ADC,
        .description = "16-Bit ADC with 4 Channels, Internal Reference & Oscillator",
        .kernel_driver = "ads1015",
        .valid_addrs = {0x48, 0x49, 0x4A, 0x4B, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x01}, .cmd_len = 1, .read_len = 2, .expected_val = 0x85, .mask = 0xFF, .read_offset = 0 }
        }
    },
    {
        .name = "HDC1080",
        .vendor = "Texas Instruments",
        .category = CAT_ENVIRONMENTAL,
        .description = "Low Power, High Accuracy Digital Humidity & Temperature Sensor",
        .kernel_driver = "hdc100x",
        .valid_addrs = {0x40, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0xFF}, .cmd_len = 1, .read_len = 2, .expected_val = 0x10, .mask = 0xFF, .read_offset = 0 }
        }
    },

    /* Solomon Systech OLED Displays */
    {
        .name = "SSD1306",
        .vendor = "Solomon Systech",
        .category = CAT_DISPLAY,
        .description = "128x64 Dot Matrix OLED/PLED Segment/Common Driver",
        .kernel_driver = "ssd1306",
        .valid_addrs = {0x3C, 0x3D, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 1, .expected_val = 0x03, .mask = 0x0F }
        }
    },
    {
        .name = "SH1106",
        .vendor = "Sino Wealth",
        .category = CAT_DISPLAY,
        .description = "132x64 Dot Matrix OLED Driver with Internal Charge Pump",
        .kernel_driver = "sh1106",
        .valid_addrs = {0x3C, 0x3D, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 1, .expected_val = 0x08, .mask = 0x0F }
        }
    },

    /* Sensirion Environmental */
    {
        .name = "SHTC3",
        .vendor = "Sensirion",
        .category = CAT_ENVIRONMENTAL,
        .description = "Digital Humidity and Temperature Sensor for Low-Power IoT",
        .kernel_driver = "shtc1",
        .valid_addrs = {0x70, 0},
        .num_rules = 1,
        .rules = {
            /* Command 0xEFC8 reads ID register */
            { .type = RULE_WRITE_READ, .reg_or_cmd = {0xEF, 0xC8}, .cmd_len = 2, .read_len = 3, .expected_val = 0x07, .mask = 0x3F, .read_offset = 1 }
        }
    },
    {
        .name = "AHT20",
        .vendor = "ASAIR",
        .category = CAT_ENVIRONMENTAL,
        .description = "Integrated Temperature and Humidity Sensor",
        .kernel_driver = "aht10",
        .valid_addrs = {0x38, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x71}, .cmd_len = 1, .read_len = 1, .expected_val = 0x18, .mask = 0x18 }
        }
    },

    /* Optical & Distance */
    {
        .name = "VL53L0X",
        .vendor = "STMicroelectronics",
        .category = CAT_OPTICAL_TOF,
        .description = "Time-of-Flight (ToF) Laser-Ranging Distance Sensor",
        .kernel_driver = "vl53l0x-i2c",
        .valid_addrs = {0x29, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0xC0}, .cmd_len = 1, .read_len = 1, .expected_val = 0xEE, .mask = 0xFF }
        }
    },
    {
        .name = "APDS-9960",
        .vendor = "Broadcom / Avago",
        .category = CAT_OPTICAL_TOF,
        .description = "Digital Proximity, Ambient Light, RGB and Gesture Sensor",
        .kernel_driver = "apds9960",
        .valid_addrs = {0x39, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x92}, .cmd_len = 1, .read_len = 1, .expected_val = 0xAB, .mask = 0xFF }
        }
    },

    /* Microchip & Expanders */
    {
        .name = "MCP23017",
        .vendor = "Microchip",
        .category = CAT_EXPANDER_EEPROM,
        .description = "16-Bit I/O Expander with Serial Interface",
        .kernel_driver = "mcp23s08",
        .valid_addrs = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x00}, .cmd_len = 1, .read_len = 1, .expected_val = 0xFF, .mask = 0xFF }
        }
    },
    {
        .name = "MCP9808",
        .vendor = "Microchip",
        .category = CAT_ENVIRONMENTAL,
        .description = "+/-0.5C Maximum Accuracy Digital Temperature Sensor",
        .kernel_driver = "mcp9808",
        .valid_addrs = {0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0},
        .num_rules = 1,
        .rules = {
            { .type = RULE_READ_REG8, .reg_or_cmd = {0x06}, .cmd_len = 1, .read_len = 2, .expected_val = 0x00, .mask = 0xFF, .read_offset = 0 }
        }
    },
    {
        .name = "TCA9548A",
        .vendor = "Texas Instruments",
        .category = CAT_EXPANDER_EEPROM,
        .description = "Low-Voltage 8-Channel I2C Switch / Multiplexer",
        .kernel_driver = "pca954x",
        .valid_addrs = {0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0},
        .num_rules = 0 /* Identified via sysfs topology or manual match */
    },
    {
        .name = "AT24C32 / EEPROM",
        .vendor = "Atmel / Microchip",
        .category = CAT_EXPANDER_EEPROM,
        .description = "32K / 64K Two-Wire Serial EEPROM Memory",
        .kernel_driver = "at24",
        .valid_addrs = {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0},
        .num_rules = 0 /* Passive memory; avoid blind probing */
    }
};

static const size_t s_chip_count = sizeof(s_chip_database) / sizeof(s_chip_database[0]);

void i2c_db_init(void)
{
    /* Reserved for future dynamic database loading or plugins */
}

size_t i2c_db_get_total_chips(void)
{
    return s_chip_count;
}

const i2c_chip_info_t *i2c_db_get_chip_by_index(size_t index)
{
    if (index >= s_chip_count) return NULL;
    return &s_chip_database[index];
}

/*
 * Checks if the given address is listed in chip's valid addresses
 */
static bool is_addr_valid_for_chip(const i2c_chip_info_t *chip, uint8_t addr)
{
    for (size_t i = 0; i < sizeof(chip->valid_addrs); i++) {
        if (chip->valid_addrs[i] == 0) break;
        if (chip->valid_addrs[i] == addr) return true;
    }
    return false;
}

/*
 * Test candidate rules against device on bus
 */
static bool evaluate_rules(struct i2c_bus_ctx *ctx, uint8_t addr, const i2c_chip_info_t *chip)
{
    if (chip->num_rules == 0) {
        /* No active probe rules defined, address match only */
        return false;
    }

    for (size_t r = 0; r < chip->num_rules; r++) {
        const i2c_probe_rule_t *rule = &chip->rules[r];
        uint8_t buffer[8] = {0};

        if (rule->type == RULE_READ_REG8) {
            int ret = i2c_bus_read_reg8_buf(ctx, addr, rule->reg_or_cmd[0], buffer, rule->read_len);
            if (ret < 0) return false;

            uint8_t observed = buffer[rule->read_offset];
            if ((observed & rule->mask) != (rule->expected_val & rule->mask)) {
                return false;
            }
        }
        else if (rule->type == RULE_WRITE_READ) {
            int ret = i2c_bus_write_read(ctx, addr,
                                         rule->reg_or_cmd, rule->cmd_len,
                                         buffer, rule->read_len);
            if (ret < 0) return false;

            uint8_t observed = buffer[rule->read_offset];
            if ((observed & rule->mask) != (rule->expected_val & rule->mask)) {
                return false;
            }
        }
    }

    return true;
}

const i2c_chip_info_t *i2c_db_identify_device(struct i2c_bus_ctx *ctx, uint8_t addr)
{
    if (!ctx) return NULL;

    const i2c_chip_info_t *fallback_addr_match = NULL;

    for (size_t i = 0; i < s_chip_count; i++) {
        const i2c_chip_info_t *candidate = &s_chip_database[i];
        
        if (!is_addr_valid_for_chip(candidate, addr)) {
            continue;
        }

        /* If candidate has fingerprint rules, test them */
        if (candidate->num_rules > 0) {
            if (evaluate_rules(ctx, addr, candidate)) {
                return candidate; /* Exact fingerprint match! */
            }
        } else if (!fallback_addr_match) {
            /* Remember as fallback in case no other chip matches exact fingerprint */
            fallback_addr_match = candidate;
        }
    }

    return fallback_addr_match;
}
