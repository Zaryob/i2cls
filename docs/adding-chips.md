# Adding vendors and chip definitions

The catalog is compiled into the binary, so deployment needs no external database or JSON parser. Listing it never opens an adapter. Inspect existing entries with:

```sh
i2cls --list-vendors --json
i2cls --list-chips --vendor ti --json
```

## Manufacturer records

Add an internal enum key to `include/i2c_vendor.h` and one record to `src/i2c_vendor.c`. Choose a stable lowercase string ID, a canonical display name, and optional exact aliases terminated with `NULL`. IDs, names and aliases are matched without regard to case. Avoid ambiguous abbreviations and keep existing string IDs stable; numeric enum values are internal.

Chip definitions reference this enum key through `vendor_id`. Do not duplicate manufacturer strings in individual chip entries. The Microchip catalog entry also resolves the historical Atmel names already used by the database. Distinct existing product families such as Maxim and Analog Devices retain separate IDs.

## Chip records

Add an entry to `s_chip_database` in `src/i2c_db.c` with its name, vendor key, category, description, supported addresses and driver metadata. `kernel_device`, when provided, is an exact registered I2C client name; `kernel_driver` is the driver name. Shared driver names must not be used to identify a chip or manufacturer. Kernel names are accepted only at addresses listed for the chip. Ambiguous registrations remain unknown.

For active identification, obtain the manufacturer's datasheet and confirm:

- The ID register address, transfer protocol, response length and byte order.
- The bits that identify the chip, including any revision mask.
- Whether a register-pointer write is appropriate for the device and whether other chips sharing its address could interpret it differently.

Record identification rules only when they can distinguish the chip. There are at most three rules per entry, each with up to eight response bytes. Every rule must receive its complete response, and all rules must match. A power-on configuration, common zero value, address match or mutable status bit is insufficient. Use `num_rules = 0` for metadata-only records. EEPROM addresses `0x50..0x5f` are deliberately excluded from signature reads.

A catalog entry is not evidence that the chip has been tested on physical hardware. `signature_supported` describes the presence of identification rules. Scanned `vendor_source` instead describes the evidence for a particular result: `signature`, `kernel`, or `null`.

## Verification

Run `make check`. The unit suite verifies vendor references, unique IDs and unambiguous aliases. The identification tests inject correct, incorrect, short and failed replies, and check that filtering out a vendor does not read its signature registers. Add similar responses for each new rule in `tests/test_db.c`; do not rely solely on a successful mock.

Use the CLI regressions to verify text and JSON catalogs, alias filtering, metadata-only entries, kernel ownership and unknown-device behavior. Validate the actual device separately when hardware is available, and record that limitation when it is not.
