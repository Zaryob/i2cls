# i2c-tools-ng

Linux I2C veriyollarını keşfetmek, topolojiyi incelemek ve çekirdeğe cihaz eklemek için C99 komut satırı araçları.

[English](README.md) | [Türkçe](README.tr.md)

## Derleme ve test

C derleyicisi ve `make` gerekir. Gerçek donanıma erişim için Linux, `i2c-dev` ve `/dev/i2c-N` üzerinde okuma/yazma yetkisi gerekir. Genellikle kullanıcıyı `i2c` grubuna eklemek yeterlidir; grup değişikliği için yeniden oturum açılır.

```sh
make
make test                     # C birim testleri; fiziksel donanıma erişmez
make check                    # Komut satırı regresyonları da çalışır; Python 3 gerekir
make install PREFIX="$HOME/.local"
# Sistem geneli için: sudo make install
```

`make check`, Linux üzerinde geçici sysfs dizinleri ve yakalanan ioctl çağrılarıyla iç içe MUX ilişkilerini, JSON kaçışlarını, hatalı girdileri, sysfs yazma hatalarını, SMBus taramasını, repeated START okumalarını ve çekirdeğin kullandığı adresleri de doğrular. Fiziksel I2C adaptörlerini açmaz.

## Kullanım

Komutların hepsi `i2ctool` dosyasına sembolik bağlantıdır. `i2cls` ve `i2ctool ls` aynı işi yapar.

```sh
./bin/i2cls -a                 # Tüm veriyollarını tara; adres ve bilinen çekirdek adlarını göster
./bin/i2cls 1 --json           # Bir veriyolunu JSON olarak listele
./bin/i2cls --identify 1       # Register üzerinden çip tanımayı ayrıca etkinleştir
./bin/i2ctree                  # Çekirdeğe kayıtlı adaptör, cihaz ve iç içe MUX kanalları
./bin/i2ctree --json
./bin/i2cprobe 1 0x76          # Adaptör yetenekleri, sürücü durumu, ACK ve çip imzası
./bin/i2cbind --help
```

Adresler `0x03..0x77` aralığında, veriyolu numarası negatif olmayan ondalık sayı olmalıdır. Hatalı girdiler, adaptör erişim sorunları ve desteklenmeyen işlemler sıfırdan farklı çıkış kodu döndürür. Tek bir veriyolunun açılamaması JSON biçimini bozmaz; tanı mesajları stderr'e yazılır. Boruya yönlendirilen çıktıda renk kodları bulunmaz. `NO_COLOR=1` de renkleri kapatır.

`i2ctree` yalnızca sysfs okur; veriyolunda işlem başlatmaz. Linux'a önceden kayıtlı cihazları gösterir. Kayıtsız bir çevre birimi `i2cls` çıktısında bulunurken ağaçta görünmeyebilir. MUX ilişkileri `mux_device` ve `channel-N` bağlantılarından çözülür. Cihaz adı ve etkin sürücü sysfs üzerinden alınır.

## Üretici ve çip kataloğu

```sh
./bin/i2cls --list-vendors
./bin/i2cls --list-chips --vendor ti --json
./bin/i2cls 1 --vendor "Texas Instruments" --json
./bin/i2cls 1 --identify --vendor bosch
```

Ortak katalog, mevcut 32 çip kaydını 12 üretici kimliği altında toplar. Filtreler kimlik, tam ad ve listelenen alternatif adları büyük/küçük harf ayrımı olmadan kabul eder: `ti` ile `Texas Instruments` aynı üreticiyi seçer; `Atmel`, `microchip` kaydına çözülür. Katalog listeleme donanıma erişmez; masaüstünde `--mock` gerektirmez. Katalog seçenekleri tarama seçenekleriyle birleştirilemez.

`--list-chips`, adresleri, çekirdekteki cihaz adını, sürücüyü ve `signature_supported` alanını gösterir. Yalnızca açıklama ve sürücü bilgisi olan kayıtlar da listelenir; bunlar için aktif tanıma desteği iddia edilmez. Üretici toplamlarında katalogdaki çip sayısı ve imza kuralı bulunan çip sayısı ayrı verilir.

Tarama JSON çıktısında `vendor_id` sabit üretici kimliği, `vendor` ortak üretici adı, `vendor_source` ise `signature` veya `kernel` olur. Üretici bilinmiyorsa üç alan da `null` değerindedir. Çekirdek bilgisi, desteklenen adreste kayıtlı tam cihaz adından türetilir; ortak sürücü adı veya adres yeterli değildir. Bu bilgi yapılandırılmış cihazı gösterir; fiziksel imza doğrulamasından ayrıdır. `--vendor` gösterilen sonuçları ve `--identify` kullanıldığında okunacak aday imzalarını sınırlar. Tek başına imza okumayı açmaz. Üreticisi bilinmeyen cihazlar filtreli sonuçlara alınmaz; boş sonuç veriyolunda cihaz olmadığı anlamına gelmez.

Üretici ve çip ekleme adımları ile test gereksinimleri için [geliştirici rehberine](docs/adding-chips.md) bakın.

## Tarama ve çip tanıma

I2C taraması donanımla haberleşir ve her cihaz için zararsız olduğu varsayılamaz. Bağlı cihazlarını bildiğiniz veriyollarında kullanın.

Varsayılan `i2cls`, register adresi göndermeden ACK ve çekirdek sahipliği kontrolü yapar. Destekleniyorsa SMBus Quick; `0x30..0x37`, `0x50..0x5f` aralıklarında veya Quick bulunmadığında SMBus Receive Byte kullanılır. Desteklenmeyen işlem türleri ve zaman aşımı hata sayılır. bcm2835 gibi bazı adaptörlerin SMBus emülasyonu yanıtsız byte okumalarında `EIO` döndürebilir; taramada bu durum ACK yok olarak yorumlanır.

`--identify` ve `i2cprobe` çip imzalarını okumak için register adresleri de gönderir. Farklı komut protokolleri kullanan cihazların durumunu değiştirebilir. Çekirdek sürücüsünün kullandığı adreslere zorla erişilmez. EEPROM aralığı `0x50..0x5f` imza okumalarından dışlanır; bilinmeyen cihazlarda rastgele register dökümü yapılmaz.

Veritabanında çip açıklamaları ve sürücü bilgileri bulunur. Aktif tanıma yalnızca kullanılabilir imza kuralları olan girdilerde uygulanır. Adres veya açılış konfigürasyonu tek başına kesin model sayılmaz. EEPROM, OLED, RTC, MUX ve benzersiz imzası olmayan diğer cihazlar, Linux bir ad sağlamadıkça `Unknown` kalır. Çekirdek adı yapılandırılmış cihazı belirtir; fiziksel model doğrulamasıyla aynı şey değildir. Başarısız imza okuması cihazın yok olduğunu göstermez.

Normal I2C adaptörlerinde register okumaları `I2C_RDWR` ile repeated START kullanır. Tek byte register okumasında SMBus Byte Data da desteklenir. Eksik yanıtla çip eşleştirilmez.

## Çekirdeğe cihaz ekleme ve silme

```sh
# BME280 cihazını doğru I2C cihaz kimliğiyle oluştur:
sudo ./bin/i2cbind 1 0x76 bme280

# Önceden new_device üzerinden oluşturulmuş cihazı sil:
sudo ./bin/i2cunbind 1 0x76
```

`i2cbind` son parametresinde çekirdeğin **I2C cihaz kimliğini** bekler. Bu kimlik modül veya sürücü adından farklı olabilir: `bme280` cihaz kimliğini `bmp280` sürücüsü yönetir. İlgili modül bulunmalıdır; cihazın oluşturulması sürücünün bağlandığını veya donanımın eşleştiğini garanti etmez. Etkin sürücüyü `i2ctree` ile kontrol edin. `i2cunbind`, `delete_device` kullanır ve `new_device` üzerinden oluşturulmuş cihazlar içindir; Device Tree düğümlerini devre dışı bırakmaz. İşlemler sysfs yazma yetkisi gerektirir ve tamponlanmış yazma hatalarını da bildirir.

## Masaüstü simülasyonu

Simülasyon açıkça seçilir. macOS veya `I2C_FORCE_MOCK` derlemesinde `--mock` verilmezse donanım erişimi desteklenmiyor hatası alınır.

```sh
./bin/i2cls --mock --json
./bin/i2cls --mock --identify 1
./bin/i2ctree --mock
./bin/i2cprobe --mock 2 0x76
```

Bir ana veriyolu ve iki MUX kanalından oluşan simülasyonda cihaz yerleşimleri ve sürücü sahipliği komutlar arasında tutarlıdır. Simülasyon yazılım davranışını doğrular; fiziksel çip üzerinde test yapıldığı anlamına gelmez.

Linux arayüzleri için [çekirdek I2C kullanıcı alanı belgesi](https://docs.kernel.org/i2c/dev-interface.html) ve [I2C sysfs belgesi](https://docs.kernel.org/i2c/i2c-sysfs.html) temel alınmıştır.

GPL-3.0-or-later ile lisanslıdır; bkz. [LICENSE](LICENSE).
