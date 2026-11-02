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
