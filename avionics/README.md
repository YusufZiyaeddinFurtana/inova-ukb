# İnova özgün UKB — yazılım ve dijital test paketi

Bu paket **ESP32 + BMP388 + MPU9255 + NEO-M8N + SX1278** için hazırlanmış bir geliştirme sürümüdür. Uçuş karar kodu masaüstünde ve ESP32 hedefinde aynı `core/flight.hpp` dosyasından derlenir. Ana Sky Logic V2’ye kod yüklenmez veya veri bağlantısı kurulmaz.

Önce [dijital test sonuçlarını](results/SONUCLAR.md), sonra [seçimleri ve sınırları](docs/TASARIM.md) okuyun. Fiziksel ayrılma çıkışı bu sürümde kapalıdır; gösterge pini ve telemetride ayrılma isteği izlenir. Hazır uçuş sürümü veya yarışma uygunluk onayı değildir.

## İçindekiler

- `core/flight.hpp`: basınç filtresi, eğim kestirimi, quaternion ile IMU dönüşümü, uçuş durumları ve kararlar.
- `core/telemetry.hpp`: 34 baytlık sürümlü telemetri, CRC-16 ve LoRa airtime hesabı.
- `firmware/`: gerçek BMP388, MPU9255, NMEA GPS ve SX1278 için ESP32 geliştirme uygulaması.
- `tools/ExportFlight.java`: OpenRocket 24.12 ile kurtarma olayları kapalı bağımsız RK4 koşusu.
- `tools/run_sil.py`: sensör verisi üretimi, arıza ekleme, aynı C++ kodunu çalıştırma ve sonuç raporu.
- `tools/decode_packet.py`: C++ kodundan bağımsız Python telemetri çözücüsü.
- `data/roket5500ft.ork`: kullanıcının verdiği dosyanın değiştirilmemiş kopyası.
- `data/openrocket_no_recovery.csv`: OpenRocket’in dünya ivmesi ve yönelim quaternion’ları dahil yeniden üretilmiş koşusu.
- `results/`: her senaryonun girdisi, gerçek C++ çıktısı, gönderilen ve çözülen paketler; özet ve grafik.

## Bilgisayarda çalıştırma

Python 3.10+, Make ve C++17 derleyicisi gerekir. Temel testler ve senaryolar üçüncü taraf Python paketi olmadan çalışır. Grafiği üretmek için matplotlib gerekir.

```sh
cd avionics
make test
```

Bu çalışma alanında grafik ve ESP32 derleme araçları da hazırlandı:

```sh
cd avionics
.venv/bin/python tools/run_sil.py
```

Başka bilgisayarda isteğe bağlı araç kurulumu:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python tools/run_sil.py
```

Beklenen durumlar açıkça belirtilir: normal senaryoda tepe noktasından 0–3 saniye sonra tek ve kalıcı komut; emniyet/sensör arızası senaryosunda komutun engellenmesi. Bu pencere **yazılım test kabul varsayımıdır**; mekanik ayrılma süresi veya yarışma onayı değildir. Testin geçmesi bir arızada kurtarma yapıldığı anlamına gelmez.

## ESP32 için derleme

```sh
cd avionics
PLATFORMIO_CORE_DIR="$PWD/.pio-core" .venv/bin/pio run
```

Tek hedef `esp32_bench`tir. `INOVA_PHYSICAL_OUTPUT=0` derleme sırasında zorunludur. GPIO25 sürekli LOW kalır. GPIO4, mantıksal ayrılma isteği oluştuğunda HIGH olur. UART0/USB, 115200 baud ile ikili telemetri verir; yazı çıktısı değildir.

Kart gelince önce PCB/pin eşleşmesi, sensör kimlikleri, eksenler, güç, bağımsız ana UKB bağlantıları ve çıkış kutuplaması doğrulanmalıdır. Bu paketteki pinler PCB tasarımı yerine geçmez.

## OpenRocket verisini yeniden üretme

OpenRocket 24.12 JAR ve Java 17+ gerekir. Aşağıdaki `ORK_JAR` ve `JAVA17` yollarını kurulumunuza göre ayarlayın. Kaynak `.ork` değişmez; kurtarma olayları yalnız bellekte kapatılır. Geometri, motor ve kayıtlı fırlatma şartları korunur; zaman adımı 0,01 s, rüzgâr tohumu 20261017 olur.

```sh
cd avionics
ORK_JAR=/Applications/OpenRocket.app/Contents/Resources/app/jar/OpenRocket-24.12.jar
JAVA17=/Applications/OpenRocket.app/Contents/Resources/jre.bundle/Contents/Home/bin/java
javac --release 17 -cp "$ORK_JAR" -d build tools/ExportFlight.java
"$JAVA17" -Djava.awt.headless=true -cp "build:$ORK_JAR" ExportFlight data/roket5500ft.ork data/openrocket_no_recovery.csv
.venv/bin/python tools/run_sil.py
```

Komut sonrası paraşüt açılması bu koşuya geri beslenmez. Amaç açılmadan önce algoritmanın kararını bağımsız test etmektir. Komut sonrasındaki düşüşü gerçek kurtarılmış uçuş gibi yorumlamayın.
