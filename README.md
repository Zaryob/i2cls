# i2c-tools-ng

A C99 command-line toolkit for Linux I2C discovery, topology inspection and device instantiation.

[English](README.md) | [Türkçe](README.tr.md)

## Build and test

Requires a C compiler and `make`. Real hardware access requires Linux, `i2c-dev`, and read/write permission on `/dev/i2c-N` (usually membership in the `i2c` group).

```sh
make
make test                     # C unit tests, no hardware access
make check                    # Also CLI regressions; requires Python 3
make install PREFIX="$HOME/.local"
# Or: sudo make install        # /usr/local/bin
```

`make check` also tests real Linux code paths against temporary sysfs fixtures and intercepted ioctls: nested MUX discovery, JSON escaping, invalid input, failed sysfs writes, SMBus probing, repeated-start reads and kernel-owned addresses. These tests do not open physical I2C adapters.

## Commands

All commands are symlinks to `i2ctool`. Both `i2cls` and `i2ctool ls` work.

```sh
./bin/i2cls -a                 # Probe available buses; show addresses and known kernel names
./bin/i2cls 1 --json           # Machine-readable results for bus 1
./bin/i2cls --identify 1       # Opt in to register-based chip identification
./bin/i2ctree                  # Kernel-registered adapters, clients and nested MUX channels
./bin/i2ctree --json
./bin/i2cprobe 1 0x76          # Adapter capabilities, ownership, ACK and signature checks
./bin/i2cbind --help
```

Addresses must be in the ordinary 7-bit range `0x03..0x77`. Bus numbers are non-negative decimal integers. Invalid arguments, adapter failures and unsupported operations return a nonzero exit status; JSON output stays valid when an individual bus cannot be opened. Diagnostics go to stderr. Piped output has no ANSI colors; `NO_COLOR=1` also disables colors.

`i2ctree` only reads sysfs and does not generate bus traffic. It displays clients already registered with Linux; an unregistered peripheral may appear in `i2cls` but be absent from the tree. MUX relationships follow `mux_device` and `channel-N` links, including cascaded multiplexers. Device names and active drivers come from sysfs.

## Probing and identification

An I2C scan generates hardware traffic and is not universally harmless. Use it on buses whose attached devices you understand.

By default, `i2cls` checks address responses and kernel ownership without writing register indices. It uses SMBus Quick where supported and SMBus Receive Byte for `0x30..0x37`, `0x50..0x5f`, or when Quick is unavailable. Unsupported probe types and bus timeouts are errors. Some adapters, including bcm2835 SMBus emulation, report failed receive-byte probes as `EIO`; scans treat these as no ACK.

`--identify` and `i2cprobe` additionally send register indices to examine chip signatures. They can change the state of devices with different command protocols. Kernel-owned addresses are never forced open. EEPROM addresses `0x50..0x5f` are excluded from signature reads; unknown devices do not receive an arbitrary register dump.

The database includes chip descriptions and driver metadata. Only entries with usable identification rules participate in active identification. A matching address or power-on configuration alone does not establish a model: EEPROMs, OLED controllers, RTCs, multiplexers and other devices without a unique signature remain `Unknown` unless Linux supplies their name. A kernel name describes a configured client; it is distinct from a verified hardware fingerprint. A failed signature read does not prove the peripheral is absent.

For plain I2C adapters, register reads use `I2C_RDWR` with repeated START. Single-byte register reads can also use SMBus Byte Data. Short responses are rejected when matching signatures.

## Kernel device management

```sh
# Instantiate a BME280 client using its I2C device ID:
sudo ./bin/i2cbind 1 0x76 bme280

# Delete a client previously created through new_device:
sudo ./bin/i2cunbind 1 0x76
```

The last argument to `i2cbind` is the kernel's **I2C device ID**, which may differ from its module or driver name. For example, `bme280` is a device ID handled by the `bmp280` driver. The relevant module must be available; successful instantiation does not guarantee that a driver attached or that the hardware matches. Check `i2ctree` for the active driver. `i2cunbind` uses `delete_device` and is intended for clients dynamically instantiated through `new_device`; it does not disable Device Tree nodes. Both operations require sysfs write permission and report buffered write failures.

## Desktop simulation

Mock mode is explicit. On macOS or a build with `I2C_FORCE_MOCK`, omitting `--mock` reports that hardware access is unavailable instead of silently inventing hardware.

```sh
./bin/i2cls --mock --json
./bin/i2cls --mock --identify 1
./bin/i2ctree --mock
./bin/i2cprobe --mock 2 0x76
```

The simulation has a root bus and two MUX channels, with consistent device membership and kernel ownership across commands. Mock data validates software behavior; it does not establish that a physical chip has been tested.

Linux interfaces follow the [kernel userspace I2C documentation](https://docs.kernel.org/i2c/dev-interface.html) and [I2C sysfs documentation](https://docs.kernel.org/i2c/i2c-sysfs.html).

Licensed under GPL-3.0-or-later; see [LICENSE](LICENSE).
