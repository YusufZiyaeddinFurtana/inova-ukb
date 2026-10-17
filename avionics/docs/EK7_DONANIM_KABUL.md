# EK-7 donanım tamamlama ve kabul formu

Ana parçalar **Sky Logic V2, ESP32, BMP388, MPU9255, NEO-M8N, SX1278** olarak korunur. EK-7 arayüzü için gerekli dönüştürücü ve konektör eklenir. Aşağıdaki bağlantı planı geliştirme tasarımıdır; üretim/ölçüm yapılmış değildir. [Kaynak gereksinimler](EK7_GEREKSINIMLER.md): 046–051, 054–063.

## İlave parça listesi

| Parça | Adet / seçim | Kapanacak gereksinim |
|---|---|---|
| TTL↔RS232 dönüştürücü | 1 × **MAX3232E**, 3,3 V besleme, tercihen sabit kart/modül | 054–056; ESP32, RS232 gerilimine doğrudan bağlanmaz |
| D-SUB9 dişi ve sabitleme | 1; doğrudan veya kilitli JST ara kablo | 057–059 |
| Veri kablosu | TXD/RXD/GND; mekanik gerilim alıcı sabitleme, diğer pinler boş | 056/058/063 |
| USB→RS232 adaptör | 1, bilgisayardaki ön test için; UKB üzerindeki sabit dönüştürücünün yerine geçmez | 005/046/063 |
| Çıkış test kablosu | Gerçek uygulanabilir kurtarma kanalı için etiketli çift; klemense uygun uç/ferül | 060–063 |
| Ölçüm ekipmanı | Osiloskop/seri analiz, voltmetre ve akım ölçümü, uygun inert test yükü | 023/047–051/060 |

TI, MAX3232E için 3–5,5 V besleme ve 250 kbit/s belirtir; 3,3 V uygulamasında dört 0,1 µF charge-pump kapasitörü kullanılır. Besleme bypass'ı ve ESD derecesi için seçilen paketin şartları uygulanmalıdır: D/DB/PW paketlerinde belirtilen IEC ESD değeri için VCC–GND'de en az 1 µF gerekir. Hazır modülün gerçek entegresi, lojik seviyesi ve kapasite yerleşimi kontrol edilmelidir. [TI MAX3232E veri sayfası, §6/§9](https://www.ti.com/lit/ds/symlink/max3232e.pdf).

## Sabit haberleşme bağlantı planı

| Kaynak | Hedef | Açıklama |
|---|---|---|
| ESP32 GPIO4 / UART1 TX | MAX3232E DIN1 / pin11 | Lojik taraf |
| MAX3232E ROUT1 / pin12 | ESP32 GPIO35 / UART1 RX | Lojik taraf; GPIO35 yalnız giriş |
| MAX3232E DOUT1 / pin14 | UKB dişi DB9 **pin2** | EK-7 dişi görselinde UKB TXD |
| UKB dişi DB9 **pin3** | MAX3232E RIN1 / pin13 | EK-7 dişi görselinde UKB RXD |
| UKB sinyal GND / dönüştürücü GND | UKB dişi DB9 **pin5** | Haberleşme referansı |
| UKB 3,3 V / GND | MAX3232E VCC16 / GND15 | Seçilen modül/paketle doğrula |
| UKB dişi2/3/5 | Test cihazı erkek2/3/5 | Numaraya göre eşleşme; iki uçtaki TX/RX isimleri ters yönü anlatır |

Sinyal isimlerinden hareketle ikinci bir çapraz kablo eklenmemeli. Tablo, EK-7'nin dişi/erkek görseline göre aday pin planıdır; adaptörün DTE/DCE yönü ve **konektör eşleşme yüzünden** numaraları süreklilik ölçümüyle doğrulanmalıdır. Lehim tarafından bakış aynalanır. Diğer DB9 pinleri güç/RTS/CTS hattı olarak kullanılmaz.

ESP32 mevcut pinleri: IMU21/22; baro32/33; SX1278 SPI18/19/23, CS13/RST14/DIO26; GPS16/17; arm27; ayrılma25. Gösterge GPIO4'ten **GPIO2**'ye taşındı; GPIO4 EK-7 TX'e ayrıldı. GPIO2 boot strap pinidir: geliştirme kartının dahili LED'i dışında yük bağlanmaz; PCB'ye doğrudan taşınmadan boot seviyeleri incelenir. [Espressif GPIO açıklaması](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html).

## Kurtarma çıkışını kapatma koşulları

EK-7'nin **0–32 VDC / 0–10 A** sınırı gerçek test cihazı ölçüm aralığıdır; UKB'ye 32 V besleme önerisi değildir. Mevcut derlemede GPIO25 sürekli LOW, fiziksel çıkış etkinleştiren hedef yok. GPIO2'de görülen karar, test cihazının pyro girişinde ölçülmüş çıkış sayılmaz.

ÖTR'nin enerji kesilerek elektromıknatısı bırakma anlatımı ile LOW/pull-down anlatımı için gerçek sürücü şeması gerekir. İncelenecek kayıt: batarya tam dolu gerilimi; bobin direnç/indüktansı ve akımı; anahtarlama kutuplaması; flyback/clamp; çıkış süresi; ana/yedek UKB'nin birbirini beslememesini sağlayan bağlantılar. Clamp seçimi bırakma gecikmesini etkileyebilir. Yük, sigorta, iletken/konektör ve sürücü bileşenleri bu ölçümlere göre boyutlanmalı.

A1'in tek ayrılma olayının bit7 ve Pyro1/Pyro2 eşlemesi resmî teyitten sonra seçilmeli. Kullanılmayan kanala sahte açma darbesi verilmez. Gerçek şema ve ölçüm olmadan “iki çıkış bağlandı / elektromıknatıs güvenli bırakır” sonucu yazılamaz.

## Uygulanacak kabul kayıtları

Her satıra tarih, kart revizyonu, firmware hash'i, cihaz/seri numarası, ham kayıt yolu, ölçüm ve sonucu girin. Şu anda tüm fiziksel sonuçlar **BEKLİYOR**.

| Test | İşlem ve kabul ölçütü | Kayıt |
|---|---|---|
| H01 | Kart/parça kimlikleri, pinler, sabit dönüştürücü ve dişi DB9 fotoğrafı; kablo sürekliliği | BEKLİYOR |
| H02 | 3,3 V besleme / lojik ve RS232 tarafının uygun seviyeleri; TX/RX yönü ve 115200 8N1 çözümü | BEKLİYOR |
| H03 | SİT esnasında arka arkaya komutlar, son byte→işleme ≤10 ms; 1 s aktivasyon; 100 ms paket aralığı; jitter/kayıp ölçülür | BEKLİYOR |
| H04 | Paket kesme, 100 ms frame timeout; SUT geçerli veri kaybında 1000 ms kapanma; Durdur sonrası yeni paket/karar yok | BEKLİYOR |
| H05 | Üç eksende bilinen hareket ve yaklaşık 90°; 2–3 m gerçek yükseklik değişimi; canlı ölçüm/kayıtla karşılaştırma | BEKLİYOR |
| H06 | SUT sırasında gerçek sensörleri hareket ettir/kes; yalnız sentetik profil aynı kararı vermeli; arm ve mod tekrarları | BEKLİYOR |
| H07 | GPS/LoRa yükü ve eyleyici anahtarlaması altında seri haberleşme, reset, besleme düşümü ve EMI kaydı | BEKLİYOR |
| H08 | Onaylanmış inert test yükünde çıkış polaritesi/süresi; tepe gerilim ≤32 V, akım ≤10 A; yeni komut olmadan çıkış yok | BEKLİYOR |
| H09 | Ana/yedek UKB bağımsız besleme/karar ve ortak eyleyici bağlantısı; her biri tek başına doğru mekanik bırakmayı sağlar | BEKLİYOR; EK-7 dışı sistem kabulü de gerekir |
| H10 | Resmî masaüstü araç sürümüyle float/checksum/Euler/ivme/A1 kanal eşlemesi altın vektörleri, normal/anormal tekrarlar ve sahada SİT/SUT sonucu | BEKLİYOR |

## Komiteye iletilecek netleştirme metni

1. Tablo1 `8C/8E/90` ve Tablo6 `1A` checksum değerleri §3 mod256 toplamıyla nasıl hesaplanıyor? Dahil edilen baytları ve bir 36 bayt ölçüm altın vektörünü paylaşabilir misiniz?
2. Sekiz FLOAT32 little endian mı big endian mı; sensör çerçevesi 36 bayt mı? 1.0f için beklenen dört bayt nedir?
3. Euler sırası, açı sıfırı/referansı ve ivmenin özgül kuvvet/yerçekimi çıkarılmış tanımı nedir? Durağan burun yukarı kart için beklenen sekiz alanı paylaşabilir misiniz?
4. A1 tek roket paraşütü bit5/bit7 ve Pyro1/Pyro2'den hangilerine eşlenmeli? İkinci kanal kullanılmadan bırakılabilir mi?
5. SUT giriş frekansı ve başlangıç pad/kalibrasyon süresi nedir? 1000 ms timeout hangi geçerli mesajdan itibaren sayılmalı? NaN/başlangıçta hazır olmayan sensörler nasıl gösterilmeli?
6. Dişi DB9'da UKB TX pin2 / RX pin3 / GND pin5 teyidi ve enerji keserek çalışan elektromıknatıs için test cihazının beklediği elektriksel çıkış/pulse/yük nedir?

Bu metin hazırlanmıştır; kimseye otomatik gönderilmedi. Cevaplar `EK7_GEREKSINIMLER.md` Ç-01–14 kayıtlarıyla eşleştirilerek profil sabitlenmelidir.
