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

#include "i2c_types.h"
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

bool g_enable_color = true;

const char *i2c_category_to_string(i2c_category_t cat)
{
    switch (cat) {
        case CAT_IMU:
            return "IMU / Motion";
        case CAT_ENVIRONMENTAL:
            return "Environmental / Weather";
        case CAT_DISPLAY:
            return "Display Controller";
        case CAT_RTC:
            return "Real-Time Clock";
        case CAT_POWER_ADC:
            return "Power / ADC / DAC";
        case CAT_OPTICAL_TOF:
            return "Optical / ToF / Proximity";
        case CAT_EXPANDER_EEPROM:
            return "I/O Expander / EEPROM";
        case CAT_TOUCH_INPUT:
            return "Touch Controller";
        case CAT_UNKNOWN:
        default:
            return "General I2C Device";
    }
}

int i2c_parse_number(const char *text, int base, int min, int max, int *value)
{
    char *end;
    if (!text || !*text || !value || !isdigit((unsigned char)*text)) return -1;
    errno = 0;
    long n = strtol(text, &end, base);
    if (errno || *end || n < min || n > max) return -1;
    *value = (int)n;
    return 0;
}

void i2c_print_json_string(const char *text)
{
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (*p == '"' || *p == '\\') printf("\\%c", *p);
        else if (*p < 0x20) printf("\\u%04x", *p);
        else putchar(*p);
    }
    putchar('"');
}
