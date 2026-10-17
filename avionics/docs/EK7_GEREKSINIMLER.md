# EK-7 2026: gereksinimler, kabul kanıtları ve açık yorumlar

İnceleme tarihi: **17 Ekim 2026**. Bu rapor gereksinim envanteridir; uygunluk sertifikası veya donanım testi sonucu değildir. Uygulama ve test sonuçları kendi tarihli çıktılarıyla ayrıca izlenmelidir.

## Kaynak ve kapsam

- [TEKNOFEST resmî Roket Yarışması sayfası][competition], “Yarışma Dokümanları”, **EK-7 … (2026)** bağlantısı 17 Ekim 2026 tarihinde doğrulandı.
- [Resmî EK-7 DOCX][ek7] bütünü incelendi: yerel `EK7-2026.docx` içindeki tüm metin, **7 tablo ve 9 gömülü görsel** okundu; `EK7-2026.txt` ile karşılaştırıldı. Dosyanın SHA-256 değeri: `51225ee7a07d5742fde198fe6dc70121770de8939c5239d5b22e09b5abb77f2a`. DOCX uygulama meta verisi 16 sayfa bildiriyor. Aşağıdaki konumlar, farklı Word sürümlerinde kayabilen sayfa numaraları yerine bölüm, tablo ve şekil adlarıyla verildi. Son iki örnek dönüşüm bölümü dahil dosyanın sonuna kadar incelendi.
- Bağlantı, tarayıcı okuma aracında DOCX içerik türü nedeniyle görüntülenemedi; resmî sayfanın bağlantı hedefi yereldeki dosyanın indirildiği URL ile aynı. Bu araç sınırlaması belge içeriğini eksik incelemek için kullanılmadı.
- [2026 ana teknik şartname][main], sürüm tablosu **1.9 / 25.08.2026**, yalnız A1'in tek paraşüt yapısı ve EK-7'nin uygulanabilirliği gibi çapraz konular için kullanıldı. Bu rapor bütün ana şartnamenin envanteri değildir.

Tablolarda **Y** yazılım, **D** donanım/elektrik, **K** karma veya sahada gösterim, **B** resmî metinde belirsizlik olduğunu ifade eder. “Kabul kanıtı” bir şartın gerçekten kapatılması için gereken kayıttır; mevcut kanıt olduğu anlamına gelmez. Eşik/sürelerin belirsiz bırakıldığı yerlerde takımın seçimi, resmî bir sayıymış gibi sunulamaz.

## Numaralı gereksinim envanteri

Her satırın normatif kaynağı, aksi belirtilmedikçe [resmî EK-7][ek7] içindeki “Konum” sütunudur. Birden fazla yerde tekrar edilen aynı gereksinim tek kimlikle birleştirildi.

### Amaç, eksenler ve resmî değerlendirme

| Kimlik | Gereksinim | Konum | Tür | Kabul kanıtı |
|---|---|---|---|---|
| EK7-001 | Barometrik irtifa, ivme ve yönelim gibi uçuş ölçümleri toplanmalı; ana algoritmada filtrelenip işlenmeli; kurtarma başlayıncaya kadar geri besleme sürekli sürmeli. | §1.1, Şekil 1 | K | Aynı uçuş çekirdeğinin normal ve sentetik veriyle yürüdüğü iz kaydı; gerçek sensörlü kesintisiz çalışma kaydı. |
| EK7-002 | Tepe noktasına ulaşma/geçme ve önceden belirlenmiş iniş koşullarında kurtarma kararı değerlendirilerek uygun çıkış oluşturulmalı. | §1.1 | K | Normal profilin beklenen olayları, karar zamanı ve fiziksel çıkış ölçümü. EK-7 burada sabit bir irtifa/açı/süre değeri vermiyor. |
| EK7-003 | Gövde ekseni sağ elli, referansı ağırlık merkezi; **+X burun**, +Y sağ taraf, +Z aşağı yön tanımıyla uyumlu olmalı. Roll X, pitch Y, yaw Z etrafındadır. SİT/SUT ivme ve açı alanları bu eksenlerde yorumlanmalı. | §1.2, Şekil 2, Tablo 2/4 notları | K/B | Eksen dönüşümünün birim vektör testleri, sağ ellilik kontrolü ve gerçek kartın üç eksende çevrilmesi. İç çekirdeğin +Z burun seçimi dış arayüze aynen geçirilemez. |
| EK7-004 | Özgün UKB resmî alandaki SİT ve SUT değerlendirmesinden geçmeli; dijital yerel test tek başına uçuş izni sağlamaz. | §2 giriş | K | Hakem cihazı ile resmî test sonucu; başarısızlık durumunda eleme belirtiliyor. |
| EK7-005 | Takım, KTR sonrasında sağlanacağı belirtilen resmî masaüstü yazılımıyla haberleşme ve test süreçlerini önceden doğrulamaya hazır olmalı. | §2 son paragraf | K | Kullanılan resmî yazılım sürümü, ayarlar, ham paketler ve çalıştırma kaydı. Bu incelemede indirilebilir resmî uygulama dosyası doğrulanmadı. |

### Komutlar ve mod yaşam döngüsü

| Kimlik | Gereksinim | Konum | Tür | Kabul kanıtı |
|---|---|---|---|---|
| EK7-006 | UKB test arayüzünü sürekli dinlemeli; test cihazının başlatma/durdurma komutlarına göre modu seçmeli. SİT ve SUT birlikte aktif olmamalı. | §2.1, Şekil 3 | Y | Normal, başlatma beklemesi, aktif test ve paket aktarımı sırasında gelen komut testleri. Tek aktif mod koşulu şeklin ayrı dallarından çıkarılan uygulama yorumudur. |
| EK7-007 | SİT başlatma komutu **`AA 20 8C 0D 0A`** tanınmalı. | Tablo 1 | Y/B | Bu beş baytın birebir altın paket testi; checksum çelişkisi Ç-01 ile ayrıca izlenir. |
| EK7-008 | SUT başlatma komutu **`AA 22 8E 0D 0A`** tanınmalı. | Tablo 1 | Y/B | Birebir altın paket testi. |
| EK7-009 | Durdur komutu **`AA 24 90 0D 0A`** tanınmalı. | Tablo 1 | Y/B | Birebir altın paket testi; bekleyen ve aktif testlerde etkisi. |
| EK7-010 | Komut alan sırası Header, Command, Checksum, `0D`, `0A`; komutun doğruluğu başlamadan doğrulanmalı. | Tablo 1 sonrası; §2.1.2/2.1.4 | Y/B | Kesilmiş/birleşik paketler, hatalı başlık/komut/checksum/sonlandırıcı, yeniden eşzamanlama. Sonlandırıcıların veri içindeki aynı baytlarla karıştırılmadığı gösterilmeli. |
| EK7-011 | Geçerli SİT başlatma komutu onaylandıktan **1 saniye sonra** SİT aktifleşmeli. | §2.1.2 | Y | Onay zamanı ve ilk SİT paketinin zaman damgaları; bekleme sırasında Durdur testi. |
| EK7-012 | Geçerli SUT başlatma komutu onaylandıktan **1 saniye sonra** SUT aktifleşmeli. | §2.1.4 | Y | Onay/aktifleşme zamanları; beklemede veri ve Durdur davranışı. |
| EK7-013 | Durdur tüm test bayraklarını temizlemeli, testi bitirmeli, test veri gönderimini durdurmalı ve normal moda dönmeli. | §2.1, §2.1.2, §2.1.4, Şekil 3 | Y | Her modda Durdur sonrası test çıkışının kesildiği; bekleyen başlangıcın daha sonra aktifleşmediği. “Veri gönderimi” test arayüzü verisidir; uçuş LoRa akışı ayrı değerlendirilir. |
| EK7-014 | Normal moda döndükten sonra yeni test yapılabilmeli; SUT farklı profillerle tekrarlanabilmeli. | §2.1.2, §2.1.4 | Y | Güç kesmeden SİT→Durdur→SUT→Durdur→ikinci SUT dizisi; önceki sentetik uçuşun kararlarının yeni oturuma taşınmadığı. |

### SİT çıkışı

| Kimlik | Gereksinim | Konum | Tür | Kabul kanıtı |
|---|---|---|---|---|
| EK7-015 | SİT yalnız test cihazına ölçüm iletimi yapmalı; veri kaynağı gerçek sensörler olmalı. | §2.1.1, §2.1.2, Şekil 4 | K | Gerçek sensöre hareket uygulanınca verinin değişmesi; sentetik girişin SİT ölçümünün yerine geçmediği; istenmeyen kurtarma çıkışı oluşmadığı. |
| EK7-016 | Sensör paketi **`AB`** başlığıyla başlamalı, aşağıdaki sekiz FLOAT32 alanı verilen sırada taşımalı, bir checksum ve **`0D 0A`** ile bitmeli. | Tablo 2/3 | Y/B | Alan bazında bağımsız kodlayıcı/çözücü vektörleri; 36 bayt gerekçesi ve Ç-02. |
| EK7-017 | İrtifa **metre / deniz seviyesi referanslı (MSL)** olmalı. Yer başlangıcına göre AGL ile karıştırılmamalı. | Tablo 2 “İrtifa” | Y/K/B | Bilinen saha MSL referansı/atmosfer ayarı ve AGL↔MSL dönüşüm testleri. Mutlak irtifa modelinin parametreleri EK-7'de verilmemiştir. |
| EK7-018 | Basınç **mBar** olmalı; BMP388'in Pa çıkışı arayüze doğru dönüştürülmeli. | Tablo 2 “Basınç” | Y/K | 101325 Pa→1013.25 mBar gibi birim testi; ölçüm cihazıyla karşılaştırma. |
| EK7-019 | İvme X/Y/Z **m/s²** olarak ve resmî gövde eksenlerinde taşınmalı. | Tablo 2; §1.2 | Y/K/B | Eksen ve birim testleri, durağan/serbest düşüş tanımının resmî yazılımla uyuşması. Özgül kuvvet ile yerçekimi çıkarılmış ivme ayrımı metinde açık değil. |
| EK7-020 | Açı X/Y/Z **derece** olarak taşınmalı; jiroskopun derece/saniye veya rad/saniye ölçümü açı yerine gönderilemez. | Tablo 2; §1.2 | Y/K/B | Bilinen yönelimlerle karşılaştırma; Euler sırası/sıfır referansı için uyumluluk kaydı. |
| EK7-021 | Sekiz sayısal alanın her biri **4 bayt FLOAT32** olmalı. ASCII metin, float64 veya ölçekli tamsayı bu paketin yerine geçmez. | Tablo 2/3; §4 örnekler | Y/B | `sizeof(float)==4`, IEEE-754 ve bayt sırası testleri; resmî test cihazı ile karşılaştırma. |
| EK7-022 | FLOAT alanlar iki ondalık basamağa sınırlandırılmalı. | §2.1.1, Tablo 3 öncesi | Y/B | Negatif/pozitif yuvarlama örnekleri; değer iki ondalığa nicemlenip FLOAT32 kodlanmalı. Binary32'nin çoğu ondalığı tam temsil edememesi Ç-05'te açıklanır. |
| EK7-023 | SİT paketleri **10 Hz**, yani nominal 100 ms aralıkla gönderilmeli. | §2.1.1/2.1.2 | Y/K | Masaüstü saat testleri ve kartta seri hat zaman damgası; yük altında kayıp/birikme kontrolü. EK-7 jitter toleransı belirtmiyor. |
| EK7-024 | Ölçümlerin ve varsa filtrelemenin tepkisi gerçek zamanlı izlenebilmeli; ani hareket, eğim ve yaklaşık **2–3 m** irtifa değişimi testi uygulanabilmeli. | §2.1.2 son iki paragraf | K | Resmî UI grafikleri ve gerçek sensörlü video/kayıt. EK-7 belirli filtre türü/katsayısı şart koşmuyor. |

### SUT girdisi, uçuş işleme ve durum çıkışı

| Kimlik | Gereksinim | Konum | Tür | Kabul kanıtı |
|---|---|---|---|---|
| EK7-025 | SUT'ta gerçek sensör ölçümleri karar girişinden çıkarılmalı; RS232 sentetik verileri sensör ölçümü kabul edilmeli. | §2.1.3/2.1.4, Şekil 5 | Y/K | Gerçek sensör akışı kesilse/değişse de aynı sentetik profilin aynı kararı vermesi; kodda veri kaynağı tek seçim noktası. |
| EK7-026 | Sentetik girdi Tablo 4'teki **AB + sekiz FLOAT32 + checksum + 0D 0A** biçiminde alınmalı; alan sırası/birim/eksen Tablo 2 ile aynı olmalı. | §2.1.3, Tablo 4, not | Y/B | Kısmi/arka arkaya gelen 36 bayt çerçeveler, birim/eksen/float dönüşüm testleri. |
| EK7-027 | Komut başlığı **AA**, sentetik ölçüm başlığı **AB** ayrımı yapılmalı. | §2.1.3 Tablo 4 sonrası | Y | Karma komut/veri akışı ve payload içinde AA/AB/0D/0A baytlarıyla parser testi. |
| EK7-028 | Sentetik veriler aynı filtreleme ve ana uçuş algoritmasına girmeli; SUT'a özel sahte karar dizisi üretilmemeli. | §2.1.4; Şekil 5 | Y/K | Normal/SUT çekirdeğinin ortaklığı; filtreden karar çıkışına kadar veri izi. |
| EK7-029 | Normal profillerde önceden belirlenen koşullarda kurtarma/ayrılma sinyali doğru üretilmeli. | §2.1.4 sonuçlar | K | Beklenen koşul tanımı, simülasyon kararı ve kartın gerçek çıkışının test cihazında ölçülmesi. |
| EK7-030 | Ani/uygunsuz ivme, basınç değişimi gibi anormal profillerde filtreleme ve hata yönetimi değerlendirmeye dayanmalı. | §2 giriş, §2.1.4 sonuçlar | Y/K | Normal, spike, donma, eksik ve anlamsız değer profilleri; beklenen tepki her profil için açık yazılmalı. Her arızada ayrılmayı engellemek tek başına başarılı kurtarma kanıtı değildir. |
| EK7-031 | Durum paketi **yalnız SUT sırasında** gönderilmeli; SUT bitince kesilmeli. | §2.1.3 “Durum Bilgilendirmesi” | Y | Normal/SİT/bekleme/Durdur durumlarında durum paketi bulunmaması. |
| EK7-032 | Durum verisi **16 bit / 2 payload baytı** olmalı; bit anlamları Tablo 5 ile eşleşmeli. İlk sekiz bit Data1, ikinci sekiz bit Data2. | Tablo 5/6 arası | Y/B | Her bitin tek başına kodlanıp çözülmesi; Ç-04. Toplam seri paket 2 değil 6 bayttır. |
| EK7-033 | Durum paketinin alan sırası **AA, Data1, Data2, checksum, 0D, 0A** olmalı. | Tablo 6 ve sonrası | Y/B | 6 bayt altın vektörleri; tablo örneği ve checksum çelişkisi Ç-01. |
| EK7-034 | Durum bitindeki 1 ilgili aşama/işlevin aktif/işlenmiş olduğunu, 0 henüz gerçekleşmediğini göstermeli; gerçek algoritma aşamalarına dayanmalı. | Tablo 5 sonrası | Y/B | Her bitin sebep olayı ile paket zamanı eşleştirilmiş iz kaydı; latched/anlık anlamı Ç-10. |
| EK7-035 | SUT durum paketi **10 Hz** gönderilmeli; Durdur gelene kadar sürmeli; timeout güvenli sonlandırması ayrıca uygulanmalı. | §2.1.3/2.1.4 | Y/B | Sentetik veri akarken 100 ms çıkış takvimi, Durdur ve bağlantı kaybı testleri. |
| EK7-036 | Tekrarlı SUT'lar farklı normal/anormal uçuş profillerini işleyebilmeli. | §2.1.4 | Y/K | Güç döngüsüz oturum tekrarı, yeni başlangıç referansı ve temiz algoritma durumu. |

Tablo 5'in bütün durum bitleri için aşağıdaki alt gereksinimler EK7-032/034'ü ayrıntılandırır. Kaynak: [EK-7, Tablo 5][ek7].

| Kimlik | Bit | Gösterilecek olay | Kabul kanıtı / açık konu |
|---|---|---|---|
| EK7-037 | 0 | Kalkış algılandı. | Kalkış algılayıcısının olayı ile eşleşmeli. |
| EK7-038 | 1 | Motor yanma önlem süresi doldu. | Gerçek tanımlı önlem süresi ile eşleşmeli; yalnız Coast durumunu bu isimle etiketlemek yeterli değil. Süre EK-7'de sayısallaştırılmamış. |
| EK7-039 | 2 | Minimum irtifa eşiği aşıldı. | Takımın açıkça belirlediği metre eşiğiyle eşleşmeli; metindeki “…” bir sayı değildir. |
| EK7-040 | 3 | Gövde açısı veya yatay eksen ivmesi belirtilen eşiği geçti. | Takımın hangi değişken/eksen/eşik seçtiği açıklanmalı; açı ve ivme aynı büyüklük değildir. |
| EK7-041 | 4 | İrtifa alçalmaya başladı. | Gerçek alçalma koşulunun olayı ile eşleşmeli. |
| EK7-042 | 5 | Sürüklenme paraşütü açma emri oluşturuldu. | A1 tek roket paraşütüyle eşleme Ç-07'de açık. “Emir” fiziksel paraşütün açıldığının ölçümü değildir. |
| EK7-043 | 6 | İrtifa belirlenen değerin altına indi. | Açıklanmış eşik ve referans; A1 uygulaması Ç-07. |
| EK7-044 | 7 | Ana paraşüt açma emri oluşturuldu. | A1 tek paraşüt eşlemesi Ç-07. Aynı emre iki isim verilip resmî onay varmış gibi sunulmamalı. |
| EK7-045 | 8–15 | Rezerve. | Uygulama tercihi olarak sıfır tut; hata/sağlık bitlerini bu alanlara izinsiz yükleme. Rezerve bitler için resmî başka anlam yok. |

### Haberleşme, zamanlama ve bütünlük

| Kimlik | Gereksinim | Konum | Tür | Kabul kanıtı |
|---|---|---|---|---|
| EK7-046 | Test cihazıyla bağlantı **RS232** olmalı. | §2.2 ve §2.3/2.4 | D/K | Dönüştürücü devresi, fiziksel bağlantı ve gerçek RS232 cihazıyla iletişim. USB üzerindeki sanal seri test bunu tek başına kanıtlamaz. |
| EK7-047 | UART **115200 bps, parite yok, 8 data biti, 1 stop biti (8N1)** olmalı. | Tablo 7 | Y/D | Kaynak yapılandırması ve mantık analizörü/seri cihaz ayarı. |
| EK7-048 | Haberleşme **full duplex** olmalı. | Tablo 7 | Y/D | SİT/durum gönderirken gelen Durdur'un işlenmesi; iki yönlü seri hat ölçümü. RTS/CTS şartı belirtilmiyor. |
| EK7-049 | **Frame Timeout 100 ms** uygulanmalı. | Tablo 7 | Y/B | Eksik çerçevenin timeout testi; süre ölçümünün ilk/son bayta göre seçimi açıkça belgelenmeli, Ç-09. |
| EK7-050 | Komut işleme gecikmesi **≤10 ms** sınırıyla uyumlu olmalı. | §2.2 “Zamanlama ve Akış Stratejisi” | Y/K/B | Son bayt doğrulaması→komutun işlenmesi ölçümü. Metin bu değeri “test cihazımızın” özelliği diye ifade ediyor; UKB için aynı üst sınır ihtiyatlı uyumluluk hedefidir, Ç-08. |
| EK7-051 | SUT oturumu **1000 ms timeout** sonrası güvenli sonlandırma akışına geçmeli. | §2.2 “Zamanlama ve Akış Stratejisi” | Y/K/B | Geçerli veri kesildiğinde/durdurulduğunda oturumun temiz kapanması; bozuk veri akışının timeout'u sonsuz yenilememesi. Süreyi neyin yenilediği metinde açık değil, Ç-08. |
| EK7-052 | Bir bayt checksum, byte değerlerinin **mod 256 toplamı** esasına dayanmalı; alıcı doğrulamalı. | §3 | Y/B | Bağımsız vektörler ve bozuk paket reddi. Kapsama giren alanlar ve tablolardaki checksum değerleri çelişkili olduğundan Ç-01 kapanmadan resmî uyum kanıtlanamaz. CRC16 kullanılan LoRa paketi bu checksum'un yerine geçmez. |
| EK7-053 | Gelen dört bayttan FLOAT32 ve FLOAT32'den dört bayt dönüşümü desteklenmeli. | §4.2/4.3 örnekleri | Y/B | Pozitif/negatif/kesirli FLOAT32 altın vektörleri; host native endian'ına bağımlı olmayan kodlama ve belgelenmiş arayüz endian profili. |

### Donanım, kablo ve çıkışlar

| Kimlik | Gereksinim | Konum | Tür | Kabul kanıtı |
|---|---|---|---|---|
| EK7-054 | Mikrodenetleyicide donanımsal veya yazılımsal UART olmalı; TTL seviyesi uygun donanımla gerçek RS232'ye çevrilmeli. | §2.3, Şekil 7 | D | Şema/BOM, 3.3 V ESP32 tarafının uygunluğu ve RS232 seviyelerinin ölçümü. |
| EK7-055 | Dönüştürücü UKB tasarımına **sabit entegre** olmalı; MAX3232 veya uygun hazır TTL↔RS232 modülü kabul edilen örneklerdir. | §2.3, Şekil 6/7 | D | Üretim dosyası veya sabitlenmiş modül/kablo fotoğrafı; geçici haricî USB-TTL dönüştürücü bu gereksinimi karşılamaz. |
| EK7-056 | Pin sırası, elektriksel kararlılık ve elektromanyetik parazit önlemleri doğru olmalı. | §2.3 | D/K | Şema ve süreklilik kontrolü, besleme kararlılığı, RF/eyleyici anahtarlaması altında seri iletişim kaydı. EK-7 belirli EMI test standardı vermiyor. |
| EK7-057 | Test cihazına bağlanan UKB arayüzü **D-SUB9 dişi** olmalı; kartta doğrudan veya JST→D-SUB9 ara kablosuyla yapılabilir. Test cihazı tarafı erkek. | §2.4, ilk Şekil 8 | D | Konektör/kablo çizimi, fotoğraf ve pin sürekliliği. |
| EK7-058 | Yalnız RXD, TXD ve GND gerekli; TX↔RX çapraz, ortak sinyal GND doğru bağlanmalı. Diğer pinler açık bırakılabilir. | §2.4, Şekil 7/8 | D | Cihaz perspektifiyle işaretlenmiş kablo çizimi ve süreklilik testi. Konektör ön/arka bakış yönü açık olmalı. |
| EK7-059 | Gömülü D-SUB9 görselinde erkek: pin 2 RXD, pin 3 TXD, pin 5 GND; dişi: pin 2 TXD, pin 3 RXD, pin 5 GND gösteriliyor. Görseldeki pin numaraları fiziksel bağlantıyla eşleşmeli. | §2.4, ilk Şekil 8 | D/B | Test cihazı üreticisiyle pin perspektifi doğrulaması; dişi uçtan ESP32'ye kadar TX/RX süreklilik ve canlı loopback testi. Etiket yerine numara esas alınmalı; Ç-11. |
| EK7-060 | UKB kurtarma/pyro çıkışları **0–32 VDC ve 0–10 A** ölçüm sınırları içinde çalışmalı; eyleyici seçimi bunları aşmamalı. | §2.4 birinci madde | D/K | Yük altında gerilim/akım ve anahtarlama geçişi ölçümü; yalnız nominal pil değeri yeterli değil. EK-7 bunu çıkış sınırı olarak verir, test cihazının UKB'yi bu gerilimde beslediği söylenmiyor. |
| EK7-061 | Kurtarma çıkışları test cihazı klemenslerine bağlanabilir kablo uçlarıyla hazırlanmalı; sağlamlık kontrol edilmeli. | §2.1.2/2.1.4 ilk maddeler, §2.4 | D | Kablo/terminal planı, polarite ve bağlantı kontrol kaydı. |
| EK7-062 | Test düzeneği RS232 yanında **Pyro 1 ve Pyro 2** kurtarma sinyal bağlantılarını da destekliyor; uygulanabilir kanallar test cihazında ölçülmeli. | §2.4, ikinci Şekil 8 | D/K/B | A1 kanal eşlemesi ve test cihazında ölçüm; ikinci fiziksel çıkışın A1'e zorunluluğu metin/ana şartname birlikte yorumlanmalı, Ç-07. |
| EK7-063 | SİT ve SUT öncesi RS232 ve kurtarma kablolarının sağlamlığı/doğruluğu kontrol edilmeli. | §2.1.2/2.1.4 ilk maddeler | K | Tekrarlanabilir bağlantı kontrol formu ve gerçek test düzeni fotoğrafı. |

## Tam paket yerleşimi

Kaynak: [EK-7 Tablo 1–6 ve §4][ek7]. Aşağıdaki sayısal yerleşim Tablo 2'deki alan boyutlarından türetilmiştir; yeni bir resmî düzeltme değildir.

| 0 tabanlı ofset | Uzunluk | SİT çıkışı / SUT girdisi |
|---|---|---|
| 0 | 1 | `AB` |
| 1 | 4 | İrtifa, metre MSL |
| 5 | 4 | Basınç, mBar |
| 9 | 4 | İvme X, m/s² |
| 13 | 4 | İvme Y, m/s² |
| 17 | 4 | İvme Z, m/s² |
| 21 | 4 | Açı X, derece |
| 25 | 4 | Açı Y, derece |
| 29 | 4 | Açı Z, derece |
| 33 | 1 | Checksum; kapsam Ç-01'de açık |
| 34 | 1 | `0D` |
| 35 | 1 | `0A` |

Toplam **1 + 8×4 + 1 + 2 = 36 bayt**. Metinde tekrarlanan “Byte 32 / Byte 33” satır etiketleri son üç baytın alanlarını iptal etmez. 34 baytlık mevcut LoRa paketi farklı bir protokoldür.

Durum için 16 bit mantıksal `status` değerinde metnin “ilk 8 bit” tanımı izlenirse `Data1 = status & 0xff`, `Data2 = (status >> 8) & 0xff` olur. Paket 6 bayttır. Örneğin sadece bit 2 ve 4 aktifse Data1=`14`, Data2=`00`; Tablo 6 da bu iki veriyi gösterir. Bu örneğin checksum'u **1A** olarak basılmıştır; hiçbir yaygın düz mod-256 alan toplamıyla uyuşmaması ayrıca kaydedilmiştir.

## Çelişki ve belirsizlik kaydı

Bu tablo yazılımın varsayım seçmesine izin verir; varsayımın resmî onayla karıştırılmasına izin vermez. Öneriler araştırmacı değerlendirmesidir.

| Kayıt | Bulgular ve etkisi | Uygulanabilecek geçici yaklaşım / kapanma kanıtı |
|---|---|---|
| **Ç-01 — Checksum örnekleri ve kapsam** | §3 yalnız byte toplamı mod 256 diyor; hangi alanların dahil olduğu yazmıyor. Tablo 1'in SİT örneğinde header+command=`CA`; bunlara footers eklenirse `E1`; basılı checksum=`8C`. SUT/Durdur için aynı iki toplam `CC/CE`, footers dahil `E3/E5`; basılı `8E/90`. Tablo 6 için header+data=`BE`, footers dahil=`D5`; basılı `1A`. Bu yüzden “footer dahil” veya “yalnız payload” sonucu resmî metinden kesin çıkarılamaz. | Komutlar için Tablo 1'in üç literalini tanıyan, checksum algoritması/alan kapsamı açıkça yapılandırılmış sürüm yapılabilir. Çift profil kullanılıyorsa hangi profilde hangi paketin kabul edildiği raporlanmalı; rastgele her checksum'u kabul etmek uygun çözüm değil. Resmî masaüstü uygulaması veya komite altın vektörleriyle çözülmeli. |
| **Ç-02 — Sensör paketi numaralandırması** | Tablo 3/4 son satırlarda 31,32,33,32,33,34 yazıyor. Sekiz FLOAT32 ve çerçeve alanları **36 bayt** ediyor. | Alan boyutuna göre 36 bayt, checksum 34. bayt, CR/LF 35/36. bayt seçimi güçlü türetimdir; uygulama/protokol testiyle doğrulanmalı. |
| **Ç-03 — FLOAT endian** | Komut ve durum alan sırası “Big Endian” diye anlatılıyor; her alan 1 bayt olduğundan bu ifade float bayt sırasını belirlemiyor. §4 union `array[0..3]` kopyası host belleğine bağlı; platform belirtilmemiş. ESP32/laptop için little endian çalışması bir çıkarımdır. | Native belleği wire'a dökmek yerine açık little/big endian kodlayıcı ve sabit test vektörü; kullanılan profil görünür olmalı. Örn. 1.0f LE=`00 00 80 3F`, BE=`3F 80 00 00`. Resmî araçla seçimi kapat. |
| **Ç-04 — Durum bayt sırası** | Metin ilk 8 bit Data1 diyor; bu bit0–7 önce anlamına gelir. Aynı bölüm “Big Endian” sözünü header→data alan sırası için kullanıyor. | Tablo 5 ve ilk 8 bit cümlesine göre low byte önce; yorum açıkça yazılmalı. Her tek bitin resmî cihazda doğru etiketi yaktığını doğrula. |
| **Ç-05 — FLOAT32 / iki ondalık** | İki ondalıklı sayı binary32'de çoğunlukla tam temsil edilemez; “yalnız 2 basamak kabulü” alan boyutunu metne dönüştürmez. | `round(x*100)/100` benzeri nicemleme, sonra binary32. Nicemleme ve tie kuralı belgelenmeli; ASCII veya fixed-point'e sessiz geçiş yapma. |
| **Ç-06 — Yönelim ve ivme tanımı** | Eksenler var; Euler dönüş sırası, mutlak açı sıfırı, body/world referansı, özgül kuvvet/yerçekimsiz ivme ayrımı eksik. İç çekirdekte +Z burun, resmî arayüzde +X burun. Jiroskop alanı hiç yok. | Vektörler için sağ elli `iç(x,y,z)=resmî(y,z,x)` dönüşümü kullanılabilir. Euler bileşenlerini yalnız bu permütasyonla taşımak genel yönelim dönüşümü değildir; quaternion/matris dönüşümü ve resmî Euler tanımı gerekir. SUT açılarını gyro hızıymış gibi kullanma. SİT'te üç açıyı sıfır doldurmak gerçek yönelim ölçümü değildir. |
| **Ç-07 — A1 tek paraşüt / genel iki kanal** | [Ana şartname §4.1.13, s.18][main] A1 roket için tek yeşil paraşüt ve ayrı görev yükü turuncu paraşütü belirtiyor. EK-7 Tablo 5 ise sürüklenme ve ana paraşüt bitleri, son şekil iki Pyro kanalı içeriyor. Bunlar A1 için ikinci roket paraşütü eklemeyi gerekçelendirmez. | Tek gerçek kurtarma olayı korunmalı. Bunun bit5/bit7/Pyro1/Pyro2'ye nasıl eşleneceği komite/resmî araçla netleştirilmeli; uygulanmayan olayın biti otomatik doğru sayılmamalı. İkinci çıkış veya ikinci komut gerekip gerekmediği açık kalır. |
| **Ç-08 — Gecikme ve timeout'un sahibi** | ≤10 ms “test cihazımızın komut işleme süresi” olarak yazılmış. 1000 ms timeout'ta hangi tarafın neyi, hangi andan itibaren beklediği ve hangi geçerli mesajın sayacı yenilediği açıklanmıyor. | UKB'de ≤10 ms servis hedefi ve geçerli sentetik veri kesilmesinden sonra ≤1000 ms güvenli test sonlandırması güçlü ihtiyatlı seçimlerdir. Komut işleme 1 s aktivasyon beklemesinden ayrı ölçülmeli. Resmî yazılımın zamanlamasıyla doğrula. |
| **Ç-09 — Frame Timeout** | 100 ms için ilk byte'tan toplam çerçeve süresi mi, ardışık byte arası boşluk mu tanımı yok. | Seçimi açıkça belgele; her ikisini sınırlandıran parser tercih edilebilir. Resmî cihazda çerçeve aralığı testi gerekir. |
| **Ç-10 — Bit ömrü ve eşikler** | Bitler “aktif” ve “henüz işlenmedi” dilleriyle tanımlanmış; geçmiş aşamaların bitlerinin tutulup tutulmayacağı açık değil. Minimum irtifa, motor önlem süresi, açı/yatay ivme ve ikinci irtifa değerleri “…” yer tutuculu. | Takım eşiklerini/config'i ve bitlerin latching davranışını yaz; algoritmanın gerçek durumundan üret. Bitleri test senaryosunun saatine göre düzmece sırayla açma. Eşikler takım uçuş tasarımından ve komiteyle uyumdan türetilmeli. |
| **Ç-11 — DB9 pin perspektifi** | Gömülü şekil dişide pin2 TXD / pin3 RXD; erkekte pin2 RXD / pin3 TXD etiketli. Bu cihaz perspektifiyle normal bir DCE/DTE eşlemesi olabilir; körlemesine aynı sinyal ismini pin numarası kabul etmek çaprazı iki kez yapabilir. | Test cihazı erkek pin2↔UKB dişi pin2, pin3↔pin3 durumunda etiketler mantıksal TX↔RX olur. Üretici pinout'u, konektörün eşleşme yüzü ve gerçek kablo sürekliliğiyle onaylanmadan kablo üretim talimatı sayılmamalı. |
| **Ç-12 — Girdi hızı / zaman / kalibrasyon** | SUT girdisinde zaman damgası yok; sentetik veri gönderim hızı sayısal olarak belirtilmemiş. 10 Hz değerleri SİT ve durum çıkışı için. Mevcut core 100 Hz IMU, 40 ms IMU tazeliği ve çok örnekli kalibrasyon istiyor; 10 Hz veriyi doğrudan vermek akışı arızaya düşürebilir. | SUT örnek zamanı/başlangıç kalibrasyonu/resampling varsayımını belirle; 10/20/50/100 Hz ve jitter testleri yap. Son ölçümü tekrar işlemek yeni fiziksel/sentetik örnek geldiği anlamına gelmemeli; veri kaybı gizlenmemeli. |
| **Ç-13 — Kurtarma çıkışının elektriksel şekli** | 32 V / 10 A sınırı var; darbe süresi, aktif seviye, low/high side, enerji keserek mekanik bırakma veya test yükünün detayı yok. Projede elektromıknatıs akımı kesilerek bırakma düşünülüyor. | Gerçek sürücü/polarite ve test cihazının ölçüm beklentisi görülmeden GPIO aktifleştirme. Sürekli LOW tutulan bench derlemesinin fiziksel testleri geçtiği söylenemez. |
| **Ç-14 — Şekil/bölüm numaraları** | İçindekilerde ikinci “2. Test Gereksinimleri” var; metin bazı şekilleri bir numara geriden çağırıyor, iki ayrı Şekil 8 başlığı var. §4.2 başlığı dönüşüm yönünü yanlış adlandırıyor. | Numaradan çok şekil başlığı/içeriğini esas al. Bu rapor iki Şekil 8'i “D-SUB9 bağlantı noktaları” ve “test düzeneği” olarak ayırır. |

## Uygulama sonrası kanıt durumu

17 Ekim 2026 uygulamasında SİT/SUT komutları, 36/6 bayt kodlayıcı/çözücü, başlangıç/timeout zamanlaması, ortak uçuş çekirdeğine sentetik giriş ve durum bitleri eklendi. Ayrıntılı uygulama ve profil [EK7_UYGULAMA.md](EK7_UYGULAMA.md), 63 kimliğin tamamının durum/evidans kaydı [EK7_DURUM.md](EK7_DURUM.md) içindedir. [Sonuç kaydı](../results/ek7/SONUCLAR.md) 18 uçtan uca dijital senaryo ve 23 protokol/oturum grubunu kapsar; önceki 55 uçuş senaryosu da yeniden geçti. ESP32 bench hedefi derlendi.

GPIO4 artık EK-7 UART TX, GPIO35 RX; gösterge GPIO2'ye taşındı. GPIO25 fiziksel ayrılma çıkışı hâlâ sürekli LOW'dur. [Donanım kabul formu](EK7_DONANIM_KABUL.md) sabit MAX3232E, dişi DB9, pyro gerilim/akım, gerçek sensör/zamanlama/EMI ve resmî cihaz doğrulamasının kapanma kanıtlarını tanımlar. Hiçbiri yalnız yazılım sonucu nedeniyle fiziksel olarak tamamlanmış sayılmaz.

## Tamamlanma için kabul katmanları

1. **Dijital yazılım:** Komut altın vektörleri, 36/6 bayt paketler, birim/eksen/float testleri, checksum profili, 1 s başlangıç, 100 ms frame ve 1000 ms oturum timeout testleri, kesintisiz dinleme, tekrarlı oturum, sentetik kaynağın tek başına karar girişi olduğu, bütün uygulanabilir durum bitleri, normal/anormal profiller ve hedef derlemesi. Her test sonuç kaydı ilgili EK7 kimliğine bağlanmalı. Uçuş algoritması aynı olmalı.
2. **Masaüstü uçtan uca:** Resmî cihaz emülatörü/araç ile gerçek binary akış; bağımsız çözücü; doğru/bozuk/eksik/birleşmiş paketler; yük altında çıktı takvimi. Yerel emülatör resmî cihaz doğrulaması değildir.
3. **Kart + seri elektrik:** ESP32, BMP388, MPU9255, NEO-M8N ve SX1278 değişmeden; gerekli RS232 dönüştürücü ve konektör eklenerek gerçek 115200 8N1 full duplex haberleşme, sensör eksenleri/yönelim/MSL ve zamanlama ölçümleri.
4. **Kurtarma çıkışı:** A1 kanal/bit eşlemesi onaylandıktan ve sürücü incelendikten sonra uygun test yüküyle gerçek gerilim/akım ve olay zamanını ölçmek. Ortak elektromıknatıs ve ana/yedek UKB bağımsızlığı ayrıca sistem gereksinimidir.
5. **Resmî uyumluluk:** Komite/masaüstü uygulaması ile Ç-01/03/04/06/07/08/09/10/11/12/13 yorumlarını kapatmak ve sahadaki SİT/SUT'u geçmek. Resmî kanıt olmadan “tüm EK-7 tamam” denemez.

## Ana şartnameyle karıştırılmaması gereken ek koşullar

[Ana şartname §4.6.9][main] EK-7'nin eksiksiz uygulanmasını ister. §4.6.12, özgün uçuş algoritmasının takım üyelerince tasarlanmasını; §4.6.14–15 sahada algoritmayı açıklayabilen ve değiştirebilen takım yetkinliğini şart koşar. Buradaki kod desteği bu takım sorumluluğunu yerine getirmiş sayılmaz. §4.6.16 ve devamındaki bağımsızlık/güç/sensör sistem koşulları ile tüm mekanik kurtarma koşulları için ayrıca ana şartname incelemesi gerekir; bu EK-7 raporu onlara uygunluk iddia etmez.

EK-7; belirli bir ESP32 modeli, LoRa/GPS modeli, zorunlu filtre yöntemi, belirli sabit kurtarma eşiği, mutlak bir test süresi veya PC emülatörünün resmî hakem cihazı sayılacağını söylemez. Bunlar takım tasarım seçimi veya ayrı doğrulama konusudur.

[competition]: https://www.teknofest.org/tr/yarismalar/roket-yarismasi/
[ek7]: https://cdn.teknofest.org/media/upload/userFormUpload/Ek-7_%C3%96zg%C3%BCn_U%C3%A7u%C5%9F_Kontrol_Bilgisayarlar%C4%B1n%C4%B1n_Tabi_Tutulaca%C4%9F%C4%B1_Testler_DtJW1.docx
[main]: https://cdn.teknofest.org/media/upload/userFormUpload/TEKNOFEST-2026_Roket_Yar%C4%B1%C5%9Fmas%C4%B1_%C5%9Eartnamesi_1CX7T.pdf
