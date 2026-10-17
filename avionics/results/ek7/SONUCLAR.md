# EK-7 yerel protokol ve SUT sonuçları

**23 test grubu; 18/18 uçtan uca senaryo geçti.**

Aynı C++ oturum/karar koduna bağımsız Python kodlayıcısının gerçek ikili paketleri verildi. Bu, resmî cihazın veya elektriksel RS232 hattının testi değildir.

![SUT karar ve durum bitleri](nominal.png)

| Senaryo | Beklenti | Sonuç | Tepeden komuta gecikme |
|---|---|---|---|
| nominal | deploy | GEÇTİ | 0.853 s |
| fragmented | deploy | GEÇTİ | 0.954 s |
| real_sensor_noise | deploy | GEÇTİ | 0.853 s |
| pressure_spike | deploy | GEÇTİ | 0.853 s |
| false_descent | deploy | GEÇTİ | 0.853 s |
| nan_pressure | deploy | GEÇTİ | 0.853 s |
| bad_checksum | deploy | GEÇTİ | 0.853 s |
| unarmed | inhibit | GEÇTİ | — |
| accel_spike | inhibit | GEÇTİ | — |
| long_gap | inhibit | GEÇTİ | — |
| pressure_frozen | inhibit | GEÇTİ | — |
| arm_lost | inhibit | GEÇTİ | — |
| altitude_frozen | inhibit | GEÇTİ | — |
| altitude_spike | deploy | GEÇTİ | 0.853 s |
| jitter | deploy | GEÇTİ | 0.860 s |
| nominal_20hz | deploy | GEÇTİ | 0.803 s |
| nominal_50hz | deploy | GEÇTİ | 0.733 s |
| nominal_100hz | deploy | GEÇTİ | 0.733 s |

Giriş 10 Hz, çıkış 10 Hz; başlatma gecikmesi 1 s, çerçeve timeout 100 ms, SUT timeout 1000 ms. SUT giriş hızı resmî belgede belirtilmediğinden 10 Hz yerel profil seçimidir.

288 ölçüm ve 40 komut tek-bit bozulması; iki float sırası; checksum seçenekleri; eksen/yönelim; mod tekrarı; bozuk ve parçalı paketler; gerçek sensör izolasyonu ve 200.000 bayt rastgele akış sınandı.

Yerel varsayımlar: komutlarda Tablo 1 sabitleri, veri/durumda header+payload toplamı, little-endian float, Euler RzRyRx, A1 tek ayrılma bit7. Resmî checksum, float/Euler ve A1 çıkış eşlemesi doğrulanmalıdır.

`inhibit` kurtarmanın gerçekleşmesi anlamına gelmez. Kalıcı IMU/basınç arızası ve test bağlantı kaybında mantıksal komut engellenir. GPIO25 fiziksel olarak sürülmez; gerçek çıkış/EMI/yük/sensör testleri bekliyor.
