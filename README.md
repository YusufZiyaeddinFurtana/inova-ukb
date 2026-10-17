# İnova UKB — uçuş yazılımı ve dijital testler

ESP32, BMP388, MPU9255, NEO-M8N ve SX1278 için özgün UKB geliştirme paketi.

- [Sensörleri uyararak yapılan SİT/SUT sonuçları](avionics/results/sensors/SONUCLAR.md)
- [Sensör modelinin kapsamı ve barometre düzeltmesi](avionics/docs/SENSOR_SIMULASYONU.md)
- [Kurulum ve çalıştırma](avionics/README.md)
- [EK-7: 63 gereksinimin güncel durumu](avionics/docs/EK7_DURUM.md)
- [EK-7 uygulaması ve RS232 test aracı](avionics/docs/EK7_UYGULAMA.md)
- [EK-7 yerel protokol/SUT sonuçları](avionics/results/ek7/SONUCLAR.md)
- [Donanım ve resmî kabul için kalan işler](avionics/docs/EK7_DONANIM_KABUL.md)
- [55 senaryonun sonuçları](avionics/results/SONUCLAR.md)
- [Tasarım seçimleri ve sınırlar](avionics/docs/TASARIM.md)
- [ESP32 derleme kaydı](avionics/results/firmware-build.log)

Masaüstü simülasyonu ve ESP32 aynı C++ uçuş karar kodunu kullanır. Kaynak OpenRocket dosyasından üretilen nominal uçuşta ayrılma isteği tepe noktasından yaklaşık 0,71 s sonra oluşmuştur. 55 uçuş senaryosu ve 18 EK-7 uçtan uca senaryoda beklenen davranış doğrulandı; 23 protokol/oturum test grubu geçti. Bunlara ek 25 sensör uyarımı senaryosunda gerçek sürücüler, sanal BMP388/MPU9255 register'larıyla çalıştırıldı. Yeni testlerin yakaladığı eski barometre verisinin yeni örnek sayılması hatası giderildi; ESP32 geliştirme hedefi yeniden derlendi.

EK-7 tam metin, 7 tablo ve 9 görselle incelendi. 63 maddelik izlenebilirlik kaydı hazır. Resmî belgenin checksum/float/Euler ve A1 çıkış eşlemesi belirsizlikleri açıkça kaydedildi; seçilebilir yerel profil resmî cihazla henüz doğrulanmadı.

![Nominal uçuş](avionics/results/nominal.png)

**Geliştirme sürümüdür.** Fiziksel ayrılma çıkışı kapalıdır. Arıza senaryosunun geçmesi başarılı kurtarma anlamına gelmez. Gerçek kart/ortak eyleyici ve resmî TEKNOFEST EK-7 test arayüzü doğrulanmamıştır. Uçuş onayı olarak kullanılmamalıdır.
