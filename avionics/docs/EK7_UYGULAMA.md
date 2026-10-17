# EK-7 uygulaması ve test kullanımı

17 Ekim 2026. [63 maddelik kaynak incelemesi](EK7_GEREKSINIMLER.md), [madde bazında durum](EK7_DURUM.md) ve [gerçek test sonuçları](../results/ek7/SONUCLAR.md) birlikte okunmalıdır. Buradaki protokol, belgedeki çelişkiler açık tutularak hazırlanmış **yerel uygulama profilidir**. Resmî cihazla henüz denenmedi.

## Aynı uçuş algoritması

Normal uçuşta ESP32, BMP388 ve MPU9255 verilerini `FlightComputer` sınıfına verir. SUT'ta aynı sınıf yalnız test cihazının paketlerinden beslenir; donanım sensör kuyruğu karar girişinden çıkarılır. SİT gerçek ölçümleri kullanır, arm zorla pasiftir; ayrılma isteği üretemez. GPS/LoRa karar kaynağı olmaz. Sky Logic V2'ye bağlantı eklenmez.

SUT'ta gelen MSL irtifa ile basınç birlikte değerlendirilir. Başlangıçta ikisinin de durgun referansı alınır; basınçtan hesaplanan yükseklik ile gönderilen irtifa arasındaki fark 25 m'yi aşarsa örnek reddedilir. İrtifa aynı medyan-3 / 50 ms düşük geçiren filtre / 400 ms regresyon penceresine girer. Bu 25 m resmî sayı değildir; yerel atmosfer farklarını tolere eden başlangıç seçimidir. Gerçek ölçümlerle ayarlanmalıdır.

Gelen açılar jiroskop hızı yapılmaz: Rz(yaw)·Ry(pitch)·Rx(roll) Euler kabulüyle quaternion oluşturulur, iki koordinat tabanı da dönüştürülür ve ortak kestiriciye yönelim gözlemi olarak verilir. Açısal değişim, durgunluk ve aralık kontrolünde kullanılır. Gerçek kartta quaternion bias düzeltilmiş gyro integrasyonundan gelir; mutlak manyetik yön ölçülmez. Başlangıç referansı yerçekimi yönüdür; manyetometre kullanılmadığı için uzun süreli yaw/roll referansı drift edebilir.

İvme **özgül kuvvet** kabul edilir: pad üzerinde yaklaşık 9,797 m/s², serbest düşüşte yaklaşık 0. Resmî cihazın yerçekimi çıkarılmış ivme gönderip göndermediği teyit edilmelidir. İç eksen +Z burun; dış eksen +X burun. Vektör dönüşümü `iç(x,y,z)=dış(y,z,x)`; Euler açılarına bu permütasyon doğrudan uygulanmaz.

## Protokol profili

| Konu | Bu derlemenin açık seçimi |
|---|---|
| Fiziksel hat | Ayrı UART1; 115200, 8N1, full duplex; sabit 3,3 V RS232 dönüştürücü gerekir |
| SİT / SUT / Durdur | `AA208C0D0A` / `AA228E0D0A` / `AA24900D0A`, Tablo 1 birebir |
| Ölçüm | 36 bayt; `AB`, sekiz IEEE754 FLOAT32, checksum, CR, LF |
| Float | Little endian; iki sıralama da kodda ve testte desteklenir |
| Veri/durum checksum | Header + payload byte toplamı mod256; checksum ve footers hariç |
| Durum | 6 bayt; `AA`, bit0–7, sıfır rezerve baytı, checksum, CR, LF |
| Örnek çelişkisi | `AA1400` için bu profil `BE` üretir. Belgede basılı `1A` hesabı doğrulanamıyor |
| İki ondalık | SİT değeri `round(x*100)/100` ile nicemlenir, sonra FLOAT32; yarımlar sıfırdan uzağa yuvarlanır |
| Başlangıç | Son komut baytı doğrulandığı andan 1 s sonra; beklemede Durdur çalışır |
| Yayın | SİT ölçümü / SUT durumu 100 ms arayla; gecikmede toplu paket yığılması yok |
| Frame timeout | İlk bayttan toplam 100 ms; eksik çerçeve atılır |
| Oturum timeout | SUT aktivasyonundan veya son geçerli sentetik veriden 1000 ms sonra kapanır; NaN/hatalı paket yenilemez |
| Giriş hızı | Varsayılan 10 Hz; resmî değer değildir. 10/20/50/100 Hz ve jitter test edildi |
| Kalibrasyon | En az 2 s durgun sentetik pad; ardından ≥250 ms sağlıklı arm; örnek sayıları yapılandırılmış giriş hızına bağlı |
| MSL çıkışı | Filtrelenmiş barometrik AGL + .ork saha referansı **970 m**; gerçek atış sahasıyla değiştirilmeli |
| Veri yoksa | SİT'te eksik/eski alan NaN; açı ve irtifa başlangıç kalibrasyonu bitene kadar NaN. Resmî aracın bu gösterimi teyit edilmeli |

`firmware/ek7_profile.hpp` derleme seçenekleri: `INOVA_EK7_FLOAT_BIG=0/1`, `INOVA_EK7_CHECKSUM_FOOTERS=0/1`, `INOVA_EK7_COMMAND_POLICY=0/1/2` (tablo / prefix toplamı / footer dahil toplam), `INOVA_EK7_INPUT_HZ=5..100`. Alternatif checksum'lar otomatik denenmez; alıcı yalnız seçilmiş profili kabul eder. Test cihazı tarafındaki seçenekler aynı olmalıdır. Resmî doğrulama sonrası seçilen profil ve altın paketler kayıt altına alınmalı.

SUT veri aralığına göre yalnız örnekleme/tazelik parametreleri değişir: IMU tazeliği 4 periyot, barometre 6 periyot; gerçek kartta 40/120 ms. Uydurma ara örnek veya tekrar edilmiş ölçüm üretilmez. Sonraki paket kesintiyi açığa çıkarır; uzun IMU kaybı inertial kararı kilitler. Veri hiç dönmezse 1 s oturum timeout'u tüm test kararlarını temizler. Zaman damgası paket içinde olmadığından alım zamanı kullanılır; host gecikmesi test zamanını etkiler.

## Durum bitleri ve karar eşikleri

Bitler oturum boyunca kilitlenir; Stop/timeout veya yeni testle temizlenir. Aşağıdaki eşikler takım tasarım seçimidir, EK-7'nin sayısal talebi değildir.

| Bit | Gerçek olay / eşik |
|---|---|
| 0 | >1,8 g kalkış, ≥80 ms doğrulanmış |
| 1 | Algılanan kalkıştan **3,5 s** motor önlem süresi dolmuş; .ork motoru 3,059 s'de bitiyor |
| 2 | Uçuşta ≥50 m AGL görülmüş |
| 3 | Uçuşta burun düşeyden >45° veya yanal özgül ivme büyüklüğü >9,797 m/s² görülmüş; karar için zorunlu kapı değildir |
| 4 | Tepeye göre ≥1,5 m düşüş ve baro hız <−0,8 m/s görülmüş |
| 5 | **0:** bu A1 mimarisinde sürüklenme paraşütü komutu yok |
| 6 | Bit2 sonrasında <50 m AGL görülmüş; ikinci açma komutu değildir |
| 7 | Tek roket paraşütü için mantıksal ayrılma isteği oluşmuş |
| 8–15 | Sıfır / rezerve |

Ayrılma için ayrıca Coast, motor önlem süresi, ≥50 m AGL, 400 ms pencere içinde ≥280 ms baro trendi, IMU düşey hız <−0,5 m/s, taze/sağlıklı iki kaynak ve ≥100 ms sürekli doğrulama gerekir. A1 tek kurtarmanın **bit7 / fiziksel kanal** eşlemesi resmî teyit bekler; bit5'te sahte olay üretilmez. İki paraşütlü farklı kategori için hazır bir uçuş uygulaması değildir.

## Bilgisayarda tekrar

```sh
cd avionics
make test
# Grafik için bu çalışma alanındaki ortam:
.venv/bin/python tools/run_sil.py
.venv/bin/python tools/run_ek7.py
```

`run_ek7.py` bağımsız Python kodlayıcıyla paket üretir; hedefteki `ek7.hpp` ve `flight.hpp` ile derlenen C++ oturumuna verir. Ham olaylar, cevap baytları, durum/karar izleri `results/ek7/` altındadır. İkinci programın aynı encoder'ı paylaşarak hatayı gizlemesi önlenir. OpenRocket koşusunda kurtarma olayları kapalıdır; grafiklerde komut sonrasında paraşüt açılması yörüngeye geri beslenmez. Bu bölüm gerçek paraşütlü iniş öngörüsü değildir. `data/ek7_nominal_10hz.csv` kart geldiğinde kullanılacak sekiz alanlı sentetik profil örneğidir.

Kart geldiğinde USB→**RS232** adaptörle yerel ön test:

```sh
.venv/bin/python tools/ek7_device.py --port /dev/cu.YOUR_RS232 --mode sit --seconds 30 --output sit.jsonl
.venv/bin/python tools/ek7_device.py --port /dev/cu.YOUR_RS232 --mode sut --input data/ek7_nominal_10hz.csv --seconds 40 --output sut.jsonl
```

Bu araç resmî TEKNOFEST uygulaması değildir. RX/TX ham paket ve zamanları JSONL kaydeder; alınan ölçümleri/durumları canlı gösterir; çıkışta Durdur gönderir. SUT için kartın yerel arm girişi etkin olmalıdır. Fiziksel ayrılma GPIO25 bu bench derlemesinde sürekli LOW'dur. GPIO2 yalnız karar göstergesidir; gerçek yük bağlanmaz. USB UART0'daki 34 bayt LoRa telemetrisi ayrı kalır; `health & 0x0400` bekleyen/aktif test verisini işaretler.

Test görevi core0'da 1 ms RTOS periyoduyla ayrı çalışır; barometre okumasından ve LoRa gönderiminden bekleme almaz. UART'ta boş alan yoksa paket biriktirilmez. Bu tasarım ≤10 ms servis hedefi içindir; **gerçek komut gecikmesi/jitter/düşen paket sayısı henüz ölçülmedi**. Byte alımından sonra zaten hatta çıkmış bir paketin son bitleri Durdur sonrasında fiziksel olarak görülebilir; yeni paket kuyruğa alınmaz.
