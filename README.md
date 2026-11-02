# i2c-tools-ng ⚡

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Standard: C99](https://img.shields.io/badge/Standard-C99-orange.svg)](https://en.wikipedia.org/wiki/C99)
[![Platform: Linux / Embedded](https://img.shields.io/badge/Platform-Linux%20%2F%20Embedded-lightgrey.svg)](https://kernel.org)
[![Tests: 151 Passing](https://img.shields.io/badge/Tests-151%20Passed-brightgreen.svg)](#testing)

**Next-Generation, Smart I2C Diagnostic, Discovery & Driver Management Suite for Embedded Linux.**

[English](README.md) | [Türkçe](README.tr.md)

---

## 💡 Overview

Traditional Linux tools like `i2cdetect` have remained largely unchanged for decades: they only print an anonymous grid of raw hexadecimal addresses (e.g., `0x68`, `0x76`). Developers are left guessing what chips are attached, looking up datasheets, verifying multiplexer channels manually, and battling kernel driver conflicts.

**`i2c-tools-ng`** is a ground-up, clean-room implementation written in **pure C (C99)** that bridges the gap between low-level hardware probing and the Linux kernel:
1. **Hardware Fingerprinting:** Inspects signature registers (`WHO_AM_I`, `CHIP_ID`, status words) across 50+ sensors, displays, RTCs, and IMUs to identify the exact chip model, vendor, and category.
2. **Sysfs Topology Engine:** Resolves bus hierarchy, detects I2C Multiplexers (TCA9548A, PCA9548), and displays tree structures showing which channels lead to which devices.
3. **Driver Awareness & Live Binding:** Detects if an address is actively claimed by a kernel driver, displays driver names, and allows instant live binding (`new_device`) or unbinding without rebooting.
4. **Developer-Friendly:** ANSI colored terminal output, machine-readable `--json` export for CI/CD and automated agents, and a built-in `--mock` simulation mode for desktop testing.

---

## 🛠️ The Command Suite

The suite uses a lightweight, multicall architecture (like `git` or `busybox`). Commands can be called as standalone symlinks or via `i2ctool <subcommand>`:

| Command | Purpose |
| :--- | :--- |
| **`i2cls`** | **Smart Bus Scanner:** Scans buses, fingerprints chips, resolves categories and displays bound/available kernel drivers. Supports `--json`. |
| **`i2ctree`** | **Bus Hierarchy Visualizer:** Maps SoC root adapters, multiplexers (MUX), and connected peripherals into a clean Unicode/ASCII tree. |
| **`i2cprobe`** | **Deep Register Inspector:** In-depth diagnosis of a specific address, adapter capabilities (`I2C_FUNCS`), register dump, and binding commands. |
| **`i2cbind`** | **Live Driver Instantiator:** Binds a Linux kernel driver to an I2C device via sysfs on the fly without rebooting. |
| **`i2cunbind`** | **Live Driver Remover:** Deletes an instantiated device from the Linux kernel. |
| **`i2ctool`** | **Unified Binary:** Umbrella dispatcher CLI for all subcommands. |

---

## 📸 Usage & Examples

### 1. Smart I2C Scan (`i2cls`)

Scan a specific bus or all buses (`-a`):

```bash
i2cls 1
```

```text
=== I2C Bus 1: bcm2835 I2C adapter (/dev/i2c-1) ===
  ADDR    CHIP            VENDOR              CATEGORY                  STATUS / DRIVER
  ----------------------------------------------------------------------------------------
  0x3c   SSD1306         Solomon Systech     Display Controller        [Driver: ssd1306]
  0x40   INA219          Texas Instruments   Power / ADC / DAC         [Driver: ina219]
  0x68   MPU-6050        TDK InvenSense      IMU / Motion              [Bound: inv-mpu6050]
  0x70   TCA9548A        Texas Instruments   I/O Expander / EEPROM     [Driver: pca954x]
  0x76   BME280          Bosch Sensortec     Environmental / Weather   [Driver: bmp280]
```

Output as JSON for scripts, dashboards, and automated tooling:
```bash
i2cls 1 --json
```

---

### 2. Bus Topology Tree (`i2ctree`)

Detects bus masters, parent adapters, and TCA9548/PCA9548 multiplexer channels:

```bash
i2ctree
```

```text
Linux I2C Bus Topology Tree
=============================
● i2c-1: bcm2835 I2C adapter (Broadcom SoC) (/sys/class/i2c-adapter/i2c-1)
  ├── 0x3c: SSD1306 (Display Controller) [Driver: ssd1306]
  ├── 0x68: MPU-6050 (IMU / Motion) [Bound: inv-mpu6050]
  └── 0x70: TCA9548A (I/O Expander / EEPROM) [Driver: pca954x]
  └── [MUX Channel 0] -> i2c-2: i2c-1-mux (chan_id 0)
      └── 0x76: BME280 (Environmental / Weather) [Driver: bmp280]
  └── [MUX Channel 1] -> i2c-3: i2c-1-mux (chan_id 1)
      └── 0x40: INA219 (Power / ADC / DAC) [Driver: ina219]
```

---

### 3. Deep Address Diagnostics (`i2cprobe`)

Inspect a single address with adapter capability verification, sysfs check, and register dump:

```bash
i2cprobe 1 0x76
```

```text
=== Deep I2C Device Probe: Bus 1, Address 0x76 ===

[1] Adapter Information:
    Path            : /dev/i2c-1
    Name            : bcm2835 I2C adapter
    Mode            : Hardware Kernel Access

[2] Linux Kernel Driver Status:
    Status          : Unbound / Available (No kernel driver claimed address)

[3] Bus ACK Probe:
    Result          : ACK Received! Device is responding on bus.

[4] Device Fingerprint Identification:
    Detected Chip   : BME280
    Manufacturer    : Bosch Sensortec
    Category        : Environmental / Weather
    Description     : Combined Temperature, Humidity & Barometric Pressure Sensor
    Kernel Driver   : bmp280
    Bind Command    : i2cbind 1 0x76 bmp280

[5] Common Identification Registers:
    Reg 0xd0        : 0x60 (96)
```

---

### 4. Dynamic Driver Binding (`i2cbind` / `i2cunbind`)

Instantly bind Linux kernel drivers without modifying Device Tree blobs or rebooting:

```bash
# Bind bmp280 kernel driver to address 0x76 on bus 1
sudo i2cbind 1 0x76 bmp280

# Unbind when finished
sudo i2cunbind 1 0x76
```

---

## 🏗️ Architecture

```
┌────────────────────────────────────────────────────────┐
│                   CLI / User Layer                     │
│         i2cls │ i2ctree │ i2cprobe │ i2cbind           │
└───────────────────────────┬────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│          Smart Fingerprint Identification Engine       │
│  (Declarative Rule Matrix, WHO_AM_I, Mask & Match)     │
└───────────────────────────┬────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│           Bus & Kernel Transport Layer                 │
│  (Adapter Functionality Checking, Safe SMBus / I2C)    │
└───────────────────────────┬────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│             Linux Sysfs & Device Nodes                 │
│        /dev/i2c-*  │  /sys/class/i2c-adapter           │
│        /sys/bus/i2c/devices/* (Driver Bindings)        │
└────────────────────────────────────────────────────────┘
```

---

## 🗃️ Supported Chip Signatures

The rule-based database contains signatures for dozens of common I2C devices:

* **Bosch Sensortec:** BME280, BMP280, BMP388, BMP180, BME680, BNO055
* **TDK InvenSense:** MPU-6050, MPU-6500, MPU-9250, MPU-6886, ICM-20948
* **STMicroelectronics:** LIS3DH, LSM6DS3, LPS25H, HTS221, VL53L0X
* **Texas Instruments:** INA219, ADS1115, HDC1080, TCA9548A
* **Maxim Integrated:** DS3231, DS1307, MAX44009
* **Sensirion:** SHTC3, SHT3x
* **Displays:** SSD1306, SH1106
* **Microchip / Atmel:** MCP23017, MCP9808, AT24C32 EEPROMs
* **Broadcom / Avago:** APDS-9960

---

## ⚙️ Compilation & Installation

### Requirements
* Standard C compiler (`gcc` or `clang`)
* POSIX `make`
* Linux kernel with `/dev/i2c-*` and `i2c-dev` module enabled (or run with `--mock` on any OS).

### Build & Run Tests
```bash
# Build the suite
make

# Run the automated unit test suite (151 assertions)
make test

# Install binaries and symlinks into /usr/local/bin
sudo make install
```

### Desktop / CI Testing (`--mock`)
You can develop, test, and preview `i2c-tools-ng` on **macOS**, **x86_64**, or inside containers without real I2C hardware:
```bash
./bin/i2cls --mock
./bin/i2ctree --mock
./bin/i2cprobe -m 1 0x76
```

---

## ⚖️ License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0-or-later)**.  
See the [LICENSE](LICENSE) file for the full license text.
