# Tasarım kararları ve doğrulama sınırları

17 Ekim 2026 — Kullanıcının son talebi: ÖTR’den alınan parçalar korunsun; belirsiz uygulama seçimleri gerekçelendirilerek yapılsın. Bu belge yeni yazılım paketinin tasarım kaydıdır, kalıcı bellek notu değildir.

## Korunan mimari

Ana UKB: Sky Logic V2; özgün/yedek UKB: ESP32. Özgün kartta BMP388, MPU9255, NEO-M8N ve SX1278 korunur. Ana UKB ile elektriksel veya kablosuz haberleşme eklenmedi. Ortak elektromıknatıs sürücüsü ve yedekliliğinin doğru çalışması yazılımla kanıtlanamaz; gerçek devre şeması verilmedi.

ÖTR kaynağı: [ÖTR Final.docx](https://docs.google.com/document/d/1tn-pLzUWgnM_2C9Ej1uEmOK7opKFkprp/edit), bu konuşmada Drive üzerinden okunmuş sürüm. Yeni kullanıcı dosyası: `data/roket5500ft.ork`. Önceki klasörlerdeki farklı paraşüt/8 mm ip modelleri bu çalışmaya karıştırılmadı.

## Seçimler

| Konu | Bu sürümdeki seçim | Gerekçe / sınır |
|---|---|---|
| İşlemci kartı | ESP32-WROOM-32 tabanlı `esp32dev` geliştirme hedefi | ÖTR tam kart modelini belirtmiyor. S3/PSRAM kartına aynı pinler uygulanamaz. |
| IMU | MPU9255 kimliği 0x73; ±16 g, ±2000 °/s, 100 Hz; yazılmış I²C sürücüsü | MPU9250’a sessiz geçiş yapılmaz. Aralık taşması raporlanır ve uçuşta inertial karar kilitlenir. |
| Manyetometre | Kararda kullanılmaz | Elektromıknatıs yakınında ölçüm bozulabilir. İvme ve jiroskop kullanılmaya devam eder. |
| Barometre | BMP388, 50 Hz, basınç 2× ve sıcaklık 1× oversampling; dahili IIR kapalı | Kütüphanenin blocking okuması ayrı FreeRTOS görevinde, ayrı I²C hattında yapılır; uçuş döngüsü tek elemanlık kuyruktan beklemeden alır. Yazılımda medyan-3, 50 ms düşük geçiren filtre, 21 örneklik doğrusal eğim kestirimi. Sensörün gerçek frekans cevabı SIL’de modellenmedi. |
| Sensör yolları | MPU9255 I²C0; BMP388 I²C1 | Tek sensör hattındaki takılmanın diğer hattı doğrudan kilitlemesini azaltmak için. İşlemci/güç hâlâ ortak. |
| Kalibrasyon | En az 2 s durgun veri; yer basıncı, yerçekimi yönü/büyüklüğü, gyro ofseti | Tam 6 yüz ivme kalibrasyonu değildir. Basınç veya IMU sağlıklı değilken armlanmaz. |
| Gövde ekseni | +Z buruna doğru, sağ el koordinat sistemi | PCB montajı farklıysa veriler bu sisteme dönüştürülmeli. Quaternion body→world; dünya +Z yukarı. |
| İrtifa | Yer basıncına göre AGL; 281,845 K başlangıç sıcaklığı | .ork ISA, 970 m MSL koşuluna ait varsayım. Farklı atış sahasında yeniden ayarlanmalı. GPS irtifası kararda kullanılmaz. |
| Kalkış | >1,8 g özgül ivme, en az 80 ms doğrulama | Tek örnek darbe reddi. 80 ms tek başına uçuş/kurtarma tetiklemesi değildir. |
| Coast | Kalkıştan sonra <0,55 g özgül ivme, 150 ms doğrulama | Motor yanışı bitmeden ayrılma kararına geçilmesini engelleyen ek koşul. |
| Ayrılma | Coast + ≥50 m AGL + tepeden ≥1,5 m düşüş + barometrik hız <−0,8 m/s + IMU hızı <−0,5 m/s, 100 ms doğrulama | Sabit bir uçuş süresine veya GPS’e göre tetikleme yok. Onay süreleri sensör koşulunun devamlılığını denetler. |
| IMU hız kestirimi | Başlangıç yerçekimi yönü, bias düzeltilmiş gyro quaternion integrasyonu ve özgül ivmenin dünyaya dönüşümü | Barometre hızına geri besleme yapılmaz; ikinci kriterin sensör kaynağı ayrı tutulur. Uzun süreli gyro/ivme driftine duyarlıdır. |
| 150° koşulu | Zorunlu tetikleme koşulu olarak kaldırıldı; eğim telemetride izlenir | Verilen modelde tepe noktasında burun yaklaşık 5° düşeyden sapar. 150° beklemek kurtarmayı geciktirebilir/engelleyebilir. ÖTR’nin algoritma açıklaması bu değişiklikle güncellenmelidir. |
| LoRa | 433 MHz, SF7, BW125 kHz, CR4/5; preamble8, explicit header, CRC açık; geliştirme TX10 dBm | SF9–10 ile 5 Hz hedefi karşılanmıyordu. Yeni 34 bayt paket SF7’de yaklaşık 77,056 ms: %38,528 kanal doluluğu (5 Hz). Gerçek link bütçesi, izin verilen kullanım ve kanal planı bu hesapla doğrulanmış olmaz. |
| GPS | NEO-M8N, UART2 9600 baud, NMEA; <2 s yaşındaki fix geçerli | Varsayılan modül ayarları okunur; 5 Hz GNSS yeniden yapılandırması yapılmaz. 5 Hz telemetri, 5 Hz yeni GPS fix’i anlamına gelmez. |
| Ayrılma çıkışı | Mantıksal istek tek seferde kilitlenir; fiziksel GPIO25 kapalı | ÖTR’de enerji keserek bırakma ile bootta LOW/pull-down anlatımları çelişiyor. Elektriksel kutuplama veya fail-safe davranışı tahmin edilerek bağlanmadı. GPIO4 yalnız geliştirme göstergesi. |
| Arm | GPIO27 aktif LOW, pull-up; en az 250 ms durgun/sağlıklı veriyle armlanma | Bu giriş güç anahtarı değildir. Mevcut anahtarın uygun izole kontağı veya gerçek emniyet devresiyle tasarlanmalı. Uçuş sırasında arm kaybı yeni komutu engeller; Armed durumunda IMU akış kesintisi Fault üretir. |
| Pil ölçümü | 0 mV ve `battery unmeasured` bayrağı | Bölücü/pil şeması gelmedi; gerilim ölçülmüş gibi gösterilmez. |

Bu seçimler mevcut koşullara göre gerekçeli başlangıç değerleridir. Ölçülmüş sensör/anten/ayrılma gecikmeleri olmadan küresel olarak “en optimum” oldukları iddia edilmez.

## Önerilen pin ataması

| İşlev | ESP32 GPIO |
|---|---|
| MPU9255 SDA / SCL | 21 / 22 |
| BMP388 SDA / SCL | 32 / 33 |
| SX1278 SCK / MISO / MOSI | 18 / 19 / 23 |
| SX1278 CS / RESET / DIO0 | 13 / 14 / 26 |
| GPS RX / TX | 16 / 17 |
| Emniyet girişi | 27 |
| Ayrılma isteği — bu derlemede sürekli LOW | 25 |
| Karar göstergesi | 4 |

Pinler ESP32-WROOM geliştirme kartı varsayımıdır. Gerçek şema/PCB ve modül beslemeleriyle onaylanmadan bağlantı talimatı olarak kullanılmamalıdır. İki UKB’nin GND/güç/veri hatları bu paketle birbirine bağlanmaz.

## Arızada davranış

Basınç örnekleri geçersiz veya 120 ms’den eskiyse ayrılma koşulu doğrulanmaz. Dönüşte trend penceresi yeniden toplanır. IMU akışı uçuşta 40 ms’den uzun kesilirse veya sensör doygunluğa ulaşırsa inertial hızın güvenilirliği kaybolmuş sayılır ve yeni ayrılma komutu engellenir. Saatin geri gitmesi, uçuşta emniyet kaybı ve kalibrasyon başarısızlığı da komut üretimini engeller. Uçuşta resetten sonra otomatik kurtarma/resume yoktur.

Bu yaklaşım yanlış komut riskini azaltırken arızalı özgün UKB’nin kurtarma yapamamasına yol açabilir. Sistemin bu arızalara toleransı bağımsız ana UKB ve doğru ortak eyleyici devresine bağlıdır; ikisi de burada simüle edilmedi. Güç kaybında elektromıknatısın bırakması yazılımın garantisi değildir.

LoRa ve GPS ayrılma kararına veri vermez. LoRa asenkron gönderilir; 250 ms içinde bitmezse o radyo oturumu kapatılır. RF menzili ve gerçek sürücünün işlemciyi geciktirmemesi için kartta ölçüm gerekir.

## Dijital doğrulamanın kapsamı

SIL; gerçek OpenRocket ivme/yöneliminden body-frame özgül ivme üretir (yerçekimi ivmesi çıkarılmış ivmeölçer tanımı; serbest düşüşte yaklaşık 0 g). Gyro, ardışık body→world quaternion farkından türetilir. Kalibrasyon için uçuş öncesine 5 s durgun bölüm eklenir. Kurtarma olayları OpenRocket’in bellekteki modelinde kapatılır; RK4 adımı 10 ms, tohum sabittir. Algoritma truth irtifası/hızı/olay zamanını görmez.

Paketler C++ ile oluşturulup bağımsız Python çözücüsünden geçirilir; bütün 272 tek-bit bozulması CRC kontrolünde reddedilir. RF kaybı senaryosu bir yazılım kuyruğu/erişilebilirlik modelidir; analog SX1278 veya havadaki link simülasyonu değildir. NMEA parser ve I²C sürücüleri masaüstü core testinde çalışmaz; hedefte derlenir, fiziksel davranışları henüz test edilmedi.

Kalıcı sensör arızalarında `inhibit` beklenmesi **başarılı kurtarma** olarak raporlanmaz. Reset/brownout, EMI, kart üretimi, manyetik kilit, gerçek sensör hata dağılımları ve Sky Logic V2’nin iç yazılımı doğrulama kapsamı dışındadır. Test profili simülasyonundan resmî uygunluk sonucu çıkarılamaz. TEKNOFEST 2026 EK-7’nin resmî paket protokolü ve masaüstü test arayüzü henüz uygulanmadı.

## Resmî A1 arayüzüyle kalan farklar

Resmî 2026 EK-7 belgesi yarışma sayfasından indirildi ve metni okundu (`EK7-2026.docx`, `EK7-2026.txt`). Belgede SİT **Sensör İzleme Testi**, SUT **Sentetik Uçuş Testi** olarak tanımlanır. SİT/SUT başlatma-durdurma komutları, RS232 üzerinden 10 Hz iletişim, 1 saniyelik test başlangıç gecikmesi, test cihazının gönderdiği irtifa/basınç/ivme/açı alanları ve ayrı durum paketi gerektirir. Bu paketin LoRa telemetrisi ve masaüstü replay CSV’si bunların yerine geçmez.

EK-7’de boylamsal burun ekseni +X’tir. Çekirdeğin açıkça belirtilmiş iç koordinatı +Z burun yönündedir. Gelecek resmî arayüz adaptöründe sağ el kuralı korunarak `iç (x,y,z) = resmî (y,z,x)` dönüşümü gerekir; bu eksen eşlemesi henüz gerçek kart veya hakem cihazıyla doğrulanmadı. Gövde Euler açıları ile gyro açısal hızları aynı veri değildir; gelen açıların gyro yerine doğrudan kullanılması doğru olmaz.

Resmî örnek tablolarında byte numaralandırma tutarsızlıkları da görülüyor. Bu nedenle yalnız masaüstü SIL sonucu üzerinden “A1 testini geçer” iddiası yapılmadı. Resmî arayüz ve test veri seti ile doğrulama ayrı tamamlanmalıdır.

## Kaynaklar

- [Semtech SX1276/77/78/79 veri sayfası, s.31 airtime](https://cdn.sparkfun.com/assets/7/7/3/2/2/SX1276_Datasheet.pdf)
- [InvenSense MPU9255 üretici veri sayfası, üniversite aynası](https://web.stanford.edu/class/ee267/misc/MPU-9255-Datasheet.pdf)
- [Adafruit BMP3XX API](https://adafruit.github.io/Adafruit_BMP3XX/html/class_adafruit___b_m_p3_x_x.html)
- [LoRa kütüphanesi API](https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md)
- [TinyGPSPlus kaynak ve sürüm](https://github.com/mikalhart/TinyGPSPlus)
- [ESP32 GPIO belgeleri](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [TEKNOFEST resmî yarışma sayfası, 2026 EK-7](https://teknofest.org/tr/yarismalar/roket-yarismasi/)
- Yerel OpenRocket 24.12 JAR API’leri `javap` ile doğrulandı; `ExportFlight.java` aynı sürümle çalıştırıldı.
