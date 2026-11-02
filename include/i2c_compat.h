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

#ifndef I2C_COMPAT_H
#define I2C_COMPAT_H

#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(__linux__) && !defined(I2C_FORCE_MOCK)
    #include <linux/i2c.h>
    #include <linux/i2c-dev.h>
#else
    /* Compatibility layer for non-Linux platforms (e.g. macOS) and mock mode */
    #define I2C_SLAVE       0x0703
    #define I2C_SLAVE_FORCE 0x0706
    #define I2C_TENBIT      0x0704
    #define I2C_FUNCS       0x0705
    #define I2C_RDWR        0x0707
    #define I2C_SMBUS       0x0720

    #define I2C_FUNC_I2C                    0x00000001
    #define I2C_FUNC_10BIT_ADDR             0x00000002
    #define I2C_FUNC_PROTOCOL_MANGLING      0x00000004
    #define I2C_FUNC_SMBUS_PEC              0x00000008
    #define I2C_FUNC_NOSTART                0x00000010
    #define I2C_FUNC_SMBUS_BLOCK_PROC_CALL  0x00008000
    #define I2C_FUNC_SMBUS_QUICK            0x00010000
    #define I2C_FUNC_SMBUS_READ_BYTE        0x00020000
    #define I2C_FUNC_SMBUS_WRITE_BYTE       0x00040000
    #define I2C_FUNC_SMBUS_READ_BYTE_DATA   0x00080000
    #define I2C_FUNC_SMBUS_WRITE_BYTE_DATA  0x00100000
    #define I2C_FUNC_SMBUS_READ_WORD_DATA   0x00200000
    #define I2C_FUNC_SMBUS_WRITE_WORD_DATA  0x00400000
    #define I2C_FUNC_SMBUS_PROC_CALL        0x00800000
    #define I2C_FUNC_SMBUS_READ_BLOCK_DATA  0x01000000
    #define I2C_FUNC_SMBUS_WRITE_BLOCK_DATA 0x02000000
    #define I2C_FUNC_SMBUS_READ_I2C_BLOCK   0x04000000
    #define I2C_FUNC_SMBUS_WRITE_I2C_BLOCK  0x08000000

    #define I2C_FUNC_SMBUS_BYTE (I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE)
    #define I2C_FUNC_SMBUS_BYTE_DATA (I2C_FUNC_SMBUS_READ_BYTE_DATA | I2C_FUNC_SMBUS_WRITE_BYTE_DATA)
    #define I2C_FUNC_SMBUS_WORD_DATA (I2C_FUNC_SMBUS_READ_WORD_DATA | I2C_FUNC_SMBUS_WRITE_WORD_DATA)
    #define I2C_FUNC_SMBUS_BLOCK_DATA (I2C_FUNC_SMBUS_READ_BLOCK_DATA | I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)
    #define I2C_FUNC_SMBUS_I2C_BLOCK (I2C_FUNC_SMBUS_READ_I2C_BLOCK | I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)

    #define I2C_FUNC_SMBUS_EMUL (I2C_FUNC_SMBUS_QUICK | \
                                 I2C_FUNC_SMBUS_BYTE | \
                                 I2C_FUNC_SMBUS_BYTE_DATA | \
                                 I2C_FUNC_SMBUS_WORD_DATA | \
                                 I2C_FUNC_SMBUS_PROC_CALL | \
                                 I2C_FUNC_SMBUS_WRITE_BLOCK_DATA | \
                                 I2C_FUNC_SMBUS_I2C_BLOCK | \
                                 I2C_FUNC_SMBUS_PEC)
#endif

#endif /* I2C_COMPAT_H */
