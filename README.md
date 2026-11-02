# i2c-tools-ng (Next-Generation Linux I2C Toolkit)

**i2c-tools-ng**, modern gömülü Linux sistemleri (Raspberry Pi, BeagleBone, NVIDIA Jetson, Rockchip ve endüstriyel SBC kartları) için sıfırdan **C (C99)** ile geliştirilmiş, **GNU GPLv3** lisanslı akıllı bir I2C teşhis, tarama ve sürücü yönetim paketidir.

Geleneksel `i2c-tools` (`i2cdetect`) yalnızca ham hex adreslerini ekrana basarken; **i2c-tools-ng**, Linux `sysfs` topolojisini (Multiplexer ağaçlarını, bus hiyerarşisini) ve donanım register seviyesindeki çip parmak izlerini (`WHO_AM_I`, `CHIP_ID`) bir araya getirerek bağlı donanımı otomatik olarak tanır.

---

## 🚀 Özellikler

* **Akıllı Cihaz Tanıma (`i2cls`):** 
  Adrese yanıt veren çipin üreticisini, tam modelini (örn. *Bosch BME280*, *InvenSense MPU-6050*, *SSD1306 OLED*), cihaz kategorisini ve Linux çekirdeğindeki karşılık gelen sürücüsünü (`kernel_driver`) otomatik tespit eder.
* **Topoloji Ağacı Görünümü (`i2ctree`):**
  Sistemdeki ana I2C adaptörlerini ve I2C Multiplexer (TCA9548A / PCA9548) kanallarını hiyerarşik bir ASCII ağacı olarak modeller.
* **Derinlemesine Teşhis (`i2cprobe`):**
  Belirli bir I2C adresi üzerinde adaptör yeteneklerini (`I2C_FUNCS`), Linux sürücü bağlama durumunu ve kritik register dökümünü analiz eder.
* **Dinamik Çekirdek Sürücü Yönetimi (`i2cbind` / `i2cunbind`):**
  Sistemi yeniden başlatmaya gerek kalmadan, Linux `sysfs` (`new_device` / `delete_device`) üzerinden anında sürücü bağlar veya kaldırır.
* **Çoklu Çıktı Desteği:**
  Renkli terminal çıktısının yanı sıra CI/CD, otomasyon ve ajanlar için `--json` formatında makine tarafından okunabilir çıktı.
* **Geliştirici & Test Simülasyonu (`--mock`):**
  Fiziksel I2C donanımına sahip olmayan geliştirme ortamlarında (macOS, x86_64 masaüstü Linux, CI/CD ortamları) tam simülasyon desteği.

---

## 📦 Komutlar

| Komut | Açıklama |
| :--- | :--- |
| `i2cls [bus_nr]` | Veriyolunu akıllıca tarar, cihaz modellerini ve aktif sürücü durumlarını listeler. |
| `i2ctree` | Sistemdeki tüm I2C adaptörlerini, çoklayıcıları (MUX) ve cihazları ağaç yapısında gösterir. |
| `i2cprobe <bus> <addr>` | Belirtilen adresi derinlemesine inceler, register dökümü ve sürücü önerisi sunar. |
| `i2cbind <bus> <addr> <chip>` | Linux çekirdeğine `sysfs` üzerinden ilgili çip için sürücü bağlar. |
| `i2cunbind <bus> <addr>` | Belirtilen adresteki sürücü bağlantısını kaldırır/siler. |
| `i2ctool <komut>` | Tüm komutları tek çatı altında çalıştıran çoklu-çağrı (multicall) ikili dosyası. |

---

## 🛠️ Kurulum ve Derleme

### Gereksinimler
* C Derleyicisi (`gcc` veya `clang`)
* `make`
* Linux çekirdeği (gömülü cihazlarda `/dev/i2c-*` ve `/sys/class/i2c-adapter` etkin olmalıdır).

### Derleme ve Test
```bash
# Projeyi derle
make

# Otomatik birim testlerini çalıştır
make test

# İsteğe bağlı: Sisteme kur (/usr/local/bin)
sudo make install
```

---

## 📖 Kullanım Örnekleri

### 1. Akıllı Tarama (`i2cls`)
```bash
# i2c-1 veriyolunu tara:
i2cls 1

# Tüm I2C veriyollarını sırayla tara:
i2cls -a

# JSON çıktısı al (otomasyon ve betikler için):
i2cls --json 1
```

**Örnek Çıktı:**
```text
=== I2C Bus 1: bcm2835 I2C adapter (/dev/i2c-1) ===
  ADDR    CHIP            VENDOR              CATEGORY                  STATUS / DRIVER
  ----------------------------------------------------------------------------------------
  0x3c   SSD1306         Solomon Systech     Display Controller        [Driver: ssd1306]
  0x68   MPU-6050        TDK InvenSense      IMU / Motion              [Bound: inv-mpu6050]
  0x76   BME280          Bosch Sensortec     Environmental / Weather   [Driver: bmp280]
```

### 2. Topoloji Ağacı (`i2ctree`)
```bash
i2ctree
```
**Örnek Çıktı:**
```text
Linux I2C Bus Topology Tree
=============================
● i2c-1: bcm2835 I2C adapter (Broadcom SoC) (/sys/class/i2c-adapter/i2c-1)
  ├── 0x3c: SSD1306 (Display Controller) [Driver: ssd1306]
  ├── 0x68: MPU-6050 (IMU / Motion) [Bound: inv-mpu6050]
  └── [MUX Channel 0] -> i2c-2: i2c-1-mux (chan_id 0)
      └── 0x76: BME280 (Environmental / Weather) [Driver: bmp280]
  └── [MUX Channel 1] -> i2c-3: i2c-1-mux (chan_id 1)
      └── 0x40: INA219 (Power / ADC / DAC) [Driver: ina219]
```

### 3. Derinlemesine İnceleme (`i2cprobe`)
```bash
i2cprobe 1 0x76
```
**Örnek Çıktı:**
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

### 4. Anında Sürücü Yükleme (`i2cbind`)
```bash
# 0x76 adresindeki BME280 için kernel sürücüsünü anında bağla:
sudo i2cbind 1 0x76 bmp280

# Cihazı sistemden güvenle ayır:
sudo i2cunbind 1 0x76
```

---

## 📂 Proje Mimarisi

```
i2c-tools-ng/
├── LICENSE                    # GNU General Public License v3.0
├── Makefile                   # Otomatik derleme, test ve kurulum kuralları
├── README.md                  # Proje dokümantasyonu
├── include/
│   ├── i2c_bus.h              # I/O ve adapter yetenekleri soyutlama katmanı
│   ├── i2c_compat.h           # Taşınabilir Linux I2C ioctl tanımları & Mock katmanı
│   ├── i2c_db.h               # Deklaratif çip parmak izi veritabanı arayüzü
│   ├── i2c_sysfs.h            # Linux sysfs topoloji, MUX ve sürücü yöneticisi
│   └── i2c_types.h            # Ortak veri tipleri, renkler ve kategoriler
├── src/
│   ├── i2c_bus.c              # Donanım haberleşmesi ve güvenli register sorguları
│   ├── i2c_db.c               # Çip kural tablosu ve eşleştirme motoru
│   ├── i2c_sysfs.c            # sysfs adaptör keşfi ve new_device/delete_device
│   ├── i2c_types.c            # Tip yardımcı fonksiyonları
│   ├── main.c                 # Çoklu-çağrı (multicall) yönlendirici
│   └── commands/
│       ├── cmd_ls.c           # i2cls komutu
│       ├── cmd_tree.c         # i2ctree komutu
│       ├── cmd_probe.c        # i2cprobe komutu
│       └── cmd_bind.c         # i2cbind ve i2cunbind komutları
└── tests/
    └── test_suite.c           # 150+ birim test içeren otomatik doğrulama paketi
```

---

## ⚖️ Lisans

Bu proje **GNU General Public License v3.0 (GPL-3.0-or-later)** ile lisanslanmıştır. Detaylar için [LICENSE](LICENSE) dosyasına göz atabilirsiniz.
