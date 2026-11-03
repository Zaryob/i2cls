#!/usr/bin/env python3
"""CLI and Linux sysfs regressions. Fixtures and mocks never access real I2C."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin/i2ctool"
checks = 0


def run(*args, ok=True, binary=BIN):
    global checks
    result = subprocess.run([str(binary), *args], capture_output=True, text=True, timeout=10)
    assert (result.returncode == 0) == ok, (args, result.returncode, result.stdout, result.stderr)
    assert "\x1b" not in result.stdout, args
    checks += 1
    return result


for command in ("ls", "tree", "probe", "bind", "unbind"):
    run(command, "--help")
for command in ("ls", "tree", "probe"):
    run(command, "--bad-option", ok=False)
for bad in ("bad", "-1", "1tail", "999999999999999999999999", ""):
    run("ls", "--mock", "--", bad, ok=False)
    run("probe", "--mock", "--", bad, "0x76", ok=False)
    run("bind", bad, "0x76", "bmp280", ok=False)
    run("unbind", bad, "0x76", ok=False)
for bad in ("0", "0x02", "0x78", "0x176", "bad", "0x76tail", "-1"):
    run("probe", "--mock", "1", bad, ok=False)
    run("bind", "1", bad, "bmp280", ok=False)
    run("unbind", "1", bad, ok=False)
run("ls", "--mock", "--all", "1", ok=False)
run("tree", "--mock", "extra", ok=False)
run("probe", "--mock", "1", "0x76", "extra", ok=False)
run("probe", "--mock", "1", "0x12", ok=False)
run("probe", "--mock", "1", "0x68")  # Kernel-owned: no raw probing.
assert "Adapter Functionalities" in run("probe", "--mock", "2", "0x76").stdout
assert "BME280" in run("probe", "--mock", "2", "0x76").stdout
assert json.loads(run("ls", "--mock", "99999", "--json", ok=False).stdout) == {"buses": []}
scan = json.loads(run("ls", "--mock", "--json").stdout)["buses"]
assert [b["bus"] for b in scan] == [1, 2, 3]
assert [len(b["devices"]) for b in scan] == [5, 1, 1]
assert all(d["chip"] == "Unknown" for b in scan for d in b["devices"])
assert scan[0]["devices"][2]["active_driver"] == "inv-mpu6050"
identified = json.loads(run("ls", "1", "--mock", "--json", "--identify").stdout)
assert identified["buses"][0]["devices"][-1]["chip"] == "BME280"
assert "MUX Channel 1" in run("tree", "--mock").stdout
assert len(json.loads(run("tree", "--mock", "--json").stdout)["topology"]) == 3

# Catalog operations must succeed without --mock, sysfs or /dev access.
vendors = json.loads(run("ls", "--list-vendors", "--json").stdout)["vendors"]
chips = json.loads(run("ls", "--list-chips", "--json").stdout)["chips"]
assert len(vendors) >= 12 and len(chips) >= 32
assert len({v["id"] for v in vendors}) == len(vendors)
for vendor in vendors:
    entries = [c for c in chips if c["vendor_id"] == vendor["id"]]
    assert vendor["chip_count"] == len(entries)
    assert vendor["signature_chip_count"] == sum(c["signature_supported"] for c in entries)
    for query in (vendor["id"].upper(), vendor["name"], *vendor["aliases"]):
        filtered = json.loads(run("ls", "--list-chips", "--vendor", query, "--json").stdout)["chips"]
        assert filtered == entries
assert "SIGNATURES" in run("ls", "--list-vendors").stdout
assert "metadata" in run("ls", "--list-chips", "--vendor", "TI").stdout
for bad in ("", "Tex", "not-a-vendor"):
    result = run("ls", "--vendor", bad, "--json", ok=False)
    assert "Unknown vendor" in result.stderr and not result.stdout
for incompatible in (("--list-vendors", "--list-chips"), ("--list-chips", "--identify"),
                     ("--list-vendors", "--mock"), ("--list-chips", "--all"), ("--list-vendors", "1")):
    run("ls", *incompatible, ok=False)
run("ls", "--vendor", ok=False)
for query in ("bosch", "BOSCH SENSORTEC", "Bosch"):
    data = json.loads(run("ls", "--mock", "1", "--vendor", query, "--json").stdout)["buses"][0]["devices"]
    assert len(data) == 1 and data[0]["address"] == "0x76"
    assert data[0]["chip"] == "Unknown" and data[0]["vendor_source"] == "kernel"
data = json.loads(run("ls", "--mock", "1", "--identify", "--vendor", "bosch", "--json").stdout)["buses"][0]["devices"]
assert len(data) == 1 and data[0]["chip"] == "BME280" and data[0]["vendor_source"] == "signature"
data = json.loads(run("ls", "--mock", "1", "--identify", "--vendor", "InvenSense", "--json").stdout)["buses"][0]["devices"]
assert len(data) == 1 and data[0]["address"] == "0x68" and data[0]["bound"]
assert data[0]["vendor_source"] == "kernel" and data[0]["chip"] == "Unknown"
# The simulated pca9548 must not be mislabeled as a TI TCA9548.
data = json.loads(run("ls", "--mock", "1", "--vendor", "ti", "--json").stdout)["buses"][0]["devices"]
assert [d["address"] for d in data] == ["0x40"]
data = json.loads(run("ls", "--mock", "1", "--vendor", "solomon", "--json").stdout)["buses"][0]["devices"]
assert not data
assert all(d["vendor_id"] is None for d in scan[0]["devices"] if d["address"] in ("0x3c", "0x70"))
assert "kernel configuration" in run("probe", "--mock", "1", "0x68").stdout

if sys.platform.startswith("linux"):
    sources = [str(p) for p in sorted((ROOT / "src").rglob("*.c")) if not p.name.startswith(".")]
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = ["-std=c99", "-Wall", "-Wextra", "-Werror", "-D_DEFAULT_SOURCE",
             "-D_POSIX_C_SOURCE=200809L", "-I" + str(ROOT / "include")]
    with tempfile.TemporaryDirectory(prefix="i2cls-test-") as tmp:
        tmp = Path(tmp)
        sysfs = tmp / "devices"
        sysfs.mkdir()
        fixture_bin = tmp / "i2ctool"
        subprocess.run([*cc, *flags, '-DI2C_SYSFS_ROOT="' + str(sysfs) + '"',
                        *sources, "-o", str(fixture_bin)], check=True)

        def fixture(*args, ok=True):
            return run(*args, ok=ok, binary=fixture_bin)

        assert json.loads(fixture("tree", "--json").stdout) == {"topology": []}
        fixture("tree", ok=False)
        for nr in (99999, 100001, 100000):
            adapter = sysfs / f"i2c-{nr}"
            adapter.mkdir()
            (adapter / "name").write_text(f'Adapter "{nr}" \\ test\tname\n')
        (sysfs / "i2c-garbage").mkdir()
        device = sysfs / "99999-0050"
        device.mkdir()
        (device / "name").write_text('fixture "EEPROM"\n')
        driver = tmp / 'driver"name'
        driver.mkdir()
        (device / "driver").symlink_to(driver, target_is_directory=True)
        (sysfs / "99999-1050").mkdir()  # Unsupported 10-bit flag must not wrap to 0x50.
        for parent, child, address, channel in ((99999, 100000, "0070", 2), (100000, 100001, "0071", 7)):
            mux = sysfs / f"{parent}-{address}"
            mux.mkdir()
            (mux / "name").write_text("pca9548\n")
            (mux / f"channel-{channel}").symlink_to(sysfs / f"i2c-{child}", target_is_directory=True)
            (sysfs / f"i2c-{child}" / "mux_device").symlink_to(mux, target_is_directory=True)
        topology = json.loads(fixture("tree", "--json").stdout)["topology"]
        assert [a["bus"] for a in topology] == [99999, 100000, 100001]
        assert topology[1]["parent_bus"] == 99999 and topology[1]["mux_channel"] == 2
        assert topology[2]["parent_bus"] == 100000 and topology[2]["mux_channel"] == 7
        devices = topology[0]["devices"]
        assert len(devices) == 2 and devices[0]["name"] == 'fixture "EEPROM"'
        assert devices[0]["bound"] and devices[0]["driver"] == 'driver"name'
        assert "MUX Channel 7" in fixture("tree").stdout
        # Missing /dev nodes must produce valid empty JSON and a failure exit code.
        assert json.loads(fixture("ls", "--all", "--json", ok=False).stdout) == {"buses": []}
        for nr in (99999, 100000, 100001):
            for name in ("new_device", "delete_device"):
                (sysfs / f"i2c-{nr}" / name).symlink_to("/dev/full")
        # Buffered write errors used to be reported as successful binding/deletion.
        fixture("bind", "99999", "0x50", "24c32", ok=False)
        fixture("unbind", "99999", "0x50", ok=False)
        fixture("bind", "99999", "0x50", "bad chip", ok=False)
        transport = tmp / "test_transport"
        subprocess.run([*cc, *flags, '-DI2C_SYSFS_ROOT="' + str(sysfs) + '"',
                        str(ROOT / "tests/test_transport.c"),
                        str(ROOT / "src/i2c_bus.c"), str(ROOT / "src/i2c_sysfs.c"),
                        str(ROOT / "src/i2c_types.c"), "-Wl,--wrap=ioctl", "-o", str(transport)], check=True)
        subprocess.run([str(transport)], check=True, timeout=10)
else:
    run("ls", "1", ok=False)  # Desktop must never silently simulate hardware.
    run("tree", ok=False)

print(f"{checks} CLI/fixture scenarios passed.")
