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

#ifndef I2C_TYPES_H
#define I2C_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#define I2CTOOL_VERSION "1.0.0"

/* Device categories */
typedef enum {
    CAT_UNKNOWN = 0,
    CAT_IMU,              /* Accelerometer, Gyroscope, Magnetometer */
    CAT_ENVIRONMENTAL,    /* Temperature, Humidity, Pressure, Gas */
    CAT_DISPLAY,          /* OLED, LCD, E-Paper */
    CAT_RTC,              /* Real-Time Clock */
    CAT_POWER_ADC,        /* Current/Power Monitor, ADC, DAC */
    CAT_OPTICAL_TOF,      /* Ambient light, UV, Proximity, ToF */
    CAT_EXPANDER_EEPROM,  /* GPIO Expander, EEPROM, Memory */
    CAT_TOUCH_INPUT,      /* Capacitive touch controller */
    CAT_COUNT
} i2c_category_t;

/* Address probe status */
typedef enum {
    PROBE_NO_ACK = 0,     /* Address did not respond */
    PROBE_ACK_UNBOUND,    /* Responded, no kernel driver bound */
    PROBE_ACK_BOUND,      /* Responded, actively bound by Linux driver */
    PROBE_ERROR           /* Bus I/O error */
} i2c_probe_status_t;

/* ANSI Color support */
extern bool g_enable_color;

#define COLOR_RESET   (g_enable_color ? "\033[0m" : "")
#define COLOR_BOLD    (g_enable_color ? "\033[1m" : "")
#define COLOR_RED     (g_enable_color ? "\033[31m" : "")
#define COLOR_GREEN   (g_enable_color ? "\033[32m" : "")
#define COLOR_YELLOW  (g_enable_color ? "\033[33m" : "")
#define COLOR_BLUE    (g_enable_color ? "\033[34m" : "")
#define COLOR_MAGENTA (g_enable_color ? "\033[35m" : "")
#define COLOR_CYAN    (g_enable_color ? "\033[36m" : "")
#define COLOR_GRAY    (g_enable_color ? "\033[90m" : "")

int i2c_parse_number(const char *text, int base, int min, int max, int *value);
void i2c_print_json_string(const char *text);

const char *i2c_category_to_string(i2c_category_t cat);

#endif /* I2C_TYPES_H */
