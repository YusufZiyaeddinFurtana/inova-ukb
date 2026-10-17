# Sensörü uyararak yazılımı test etme

17 Ekim 2026 — kapsam: kullanıcının EK-7 SİT/SUT testlerini uygulama ve sensör verisini sensörün simülasyondaki uyarımından elde etme isteği. [Sonuçlar ve grafik](../results/sensors/SONUCLAR.md), [test sınırları](SENSOR_TEST_PLANI.md).

## Çalıştırılan yol

Fiziksel büyüklük → sanal sensör register'ları → I²C işlemleri → **gerçek sürücü kodu** → ortak uçuş çekirdeği → EK-7 paketi → bağımsız Python çözücüsü.

MPU9255 için gerçek `firmware/mpu9255.hpp`; BMP388 için gerçek `firmware/bmp388.hpp`, Adafruit BMP3XX **2.1.6**, Bosch BMP3 ve Adafruit BusIO **1.17.4** kaynakları masaüstünde derlenir. Aynı sürümler ESP32 hedefinde kullanılır. `third_party/LOCK.json` kopyaların hash'ini tutar; test aracı kurulu PlatformIO kaynaklarıyla da eşitliğini denetler. Kaynakların lisansları korunmuştur.

ESP32 işlemci komutları, Arduino runtime'ı ve FreeRTOS görevleri masaüstünde emüle edilmez. `Wire` sınırına bir uyumluluk katmanı konur; bu katman I²C'yi elektriksel olarak modellemez. Kullanılmayan SPI yolu seçilirse test açık hata verir. GNSS/LoRa ve manyetometre bu sensör modelinin kapsamında değildir.

## Modelde bulunanlar

| Model | Uygulanan uyarım ve sayısallaştırma |
|---|---|
| BMP388 | .ork sahasının basınç/yükseklik ilişkisi, sıcaklık, unsigned 24-bit ADC, kimlik ve kalibrasyon register'ları, trim CRC, forced conversion / DRDY, okuma ile DRDY temizlenmesi, 50 Hz host okuması |
| MPU9255 | Gövde özgül ivmesi ve açısal hız, signed 16-bit ADC, driver'ın seçtiği ±16 g / ±2000°/s aralıkları, 100 Hz, DRDY, doygunluk ve nicemleme |
| Arıza | Adres NACK, kısa I²C okuması, ADC/dönüşüm durması, yanlış chip ID, bozuk BMP388 trim CRC, aşırı ivme, tek basınç sıçraması ve geçici kesintiden dönüş |

MPU için **4 ms birinci dereceden filtre** bir model varsayımıdır; gerçek MPU9255 DSP filtresinin tam modeli değildir. Gürültülü uçuşta önceki SIL'in sabit tohumlu örnekleri kullanılır: basınç 1,5 Pa, ivme 0,025 m/s², gyro 0,0005 rad/s standart sapmalı varsayımsal gürültü. Bunlar ölçülmüş hata dağılımları değildir.

BMP388 için gerçek bir modülün fabrika trimleri yoktur. İncelenebilir, sentetik affine fixture seçildi:

- `P = 131072 − rawPressure / 128` Pa;
- `T = (rawTemperature − 5120000) / 65536` °C;
- 21 trim byte'ının bağımsız hesaplanan CRC-8 değeri `BC`.

Bu fixture gerçek vendor kalibrasyon okuma ve compensation yolunu çalıştırır; cihazın bütün doğrusal olmayan sıcaklık/gerilim çapraz duyarlılıklarını karakterize etmez. Dönüşüm süresi, kullanılan oversampling değerleriyle Bosch'un zamanlama denkleminden hesaplanır. Sonuçlar register'lara dönüşüm bitince yazılır; yeni sonuç üretilmediğinde eski değer yeni örnek sayılamaz.

## Testin yakaladığı ve giderilen hata

Önceki barometre yolunda `performReading()` başarısı yeni fiziksel ölçüm varmış gibi işaretleniyordu. Sürücü bir sonraki forced dönüşümü başlatıp veri register'larını hemen okuduğu için önceki sonuç tekrar gelebiliyordu. Dönüşümü durduran testte `!sample.baro_new` beklentisi başarısız oldu.

Yeni BMP388 adaptörü ilk dönüşümü başlatır ve ilk hazır olmayan sonucu yayımlamaz. Sonraki sonuçları ancak basınç ve sıcaklık **DRDY** bayrakları gelmişse kabul eder. Tamamlanan sonucu alırken bir sonraki dönüşüm başlatılır. Uzun kesinti sonrası geri gelen eski sonuç için ayrıca **100 ms yaş sınırı** uygulanır; eski sonuç atılırken yeni dönüşüm başlatılır. Yaş hesabı 32-bit `micros()` taşmasına uygun unsigned farkla yapılır. Bu 100 ms sensör yaş sınırı yerel seçimdir; EK-7'nin frame timeout'u ile aynı şey değildir.

Karar eşikleri bu düzeltmelerde değiştirilmedi. Sensör sürücülerini kullanan normal uçuşta nominal mantıksal istek tepeden yaklaşık **0,75 s** sonra, 10 Hz SUT'ta yaklaşık **0,85 s** sonra oluştu. Sonuçlar tam zaman damgalarıyla rapordadır.

90° çevresinde Euler açıları tekil olabilir; örneğin `[180°, 90°, 180°]` başka bir Euler gösterimiyle aynı fiziksel yönelimi ifade edebilir. Dönüş testleri bu nedenle quaternion eşdeğerliğini kontrol eder. SİT/SUT Euler tanımının hakem cihazıyla uyumluluğu ayrıca doğrulanmalıdır.

## Tekrar çalıştırma

```sh
cd avionics
make test
# Sensör testleri ve grafikleri:
.venv/bin/python tools/run_sensors.py
# Modeller + gerçek sürücüler + oturum için bellek/UB kontrolü:
python3 tools/sanitize_sensors.py
```

Grafik için matplotlib gerekir. Native C/C++ derleyici yeterlidir; sensör testleri için ESP32 kart gerekmez. PlatformIO kurulu değilse vendored native kaynaklar kullanılır; ESP32 hedef derlemesi ayrı doğrulamadır. Testler yapay zamanla hızlı çalışır, gerçek sürede uçuş beklenmez.

Sanitizer testinde upstream Adafruit sınıfının program ömrü boyunca tuttuğu ve destructor'da serbest bırakmadığı I²C tahsisleri nedeniyle **LeakSanitizer kapalıdır**. ASan ve UBSan model, gerçek sürücü ve tüm sensör senaryolarında çalışır; bellek sızıntısı olmadığı iddia edilmez. Donanım uygulaması tek global sensör örneği kullanır.

Gerçek sensörün hassasiyeti, I²C gerilimi/EMI, sensör montajı, gerçek zamanlama, RS232, güç ve ayrılma mekanizması bu dijital testle doğrulanmaz. Fiziksel GPIO25 kapalı kalır. `inhibit` sonucu arızada kurtarma yapıldığı anlamına gelmez; ana Sky Logic V2 modellenmedi. OpenRocket komut sonrası yörüngesine paraşüt açılması geri beslenmez.

## Kaynaklar

- [Bosch BMP388 veri sayfası: §3.3, §3.9–3.11 ve STATUS/data register tanımları](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf)
- [InvenSense MPU9255 üretici veri sayfası, üniversite aynası](https://web.stanford.edu/class/ee267/misc/MPU-9255-Datasheet.pdf)
- [Adafruit BMP3XX 2.1.6 kaynakları](https://github.com/adafruit/Adafruit_BMP3XX/tree/2.1.6)
- [Adafruit BusIO 1.17.4 kaynakları](https://github.com/adafruit/Adafruit_BusIO/tree/1.17.4)
