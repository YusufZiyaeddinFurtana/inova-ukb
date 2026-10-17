# İnova UKB — uçuş yazılımı ve dijital testler

ESP32, BMP388, MPU9255, NEO-M8N ve SX1278 için özgün UKB geliştirme paketi.

- [Kurulum ve çalıştırma](avionics/README.md)
- [55 senaryonun sonuçları](avionics/results/SONUCLAR.md)
- [Tasarım seçimleri ve sınırlar](avionics/docs/TASARIM.md)
- [ESP32 derleme kaydı](avionics/results/firmware-build.log)

Masaüstü simülasyonu ve ESP32 aynı C++ uçuş karar kodunu kullanır. Kaynak OpenRocket dosyasından üretilen nominal uçuşta ayrılma isteği tepe noktasından yaklaşık 0,71 s sonra oluşmuştur. 55 dijital senaryoda beklenen davranış doğrulanmış, ESP32 geliştirme hedefi derlenmiştir.

![Nominal uçuş](avionics/results/nominal.png)

**Geliştirme sürümüdür.** Fiziksel ayrılma çıkışı kapalıdır. Arıza senaryosunun geçmesi başarılı kurtarma anlamına gelmez. Gerçek kart/ortak eyleyici ve resmî TEKNOFEST EK-7 test arayüzü doğrulanmamıştır. Uçuş onayı olarak kullanılmamalıdır.
