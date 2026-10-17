# Dijital UKB test sonuçları

**55/55 senaryo beklenen davranışı gösterdi.**

Kaynak: `roket5500ft.ork`; SHA-256: `c84d12cd934ecfa5754b028fbb2d82a5292ff269afdf506df09524596f5c988f`.

Kayıtlı koşu: 1684.121 m AGL. Bağımsız, kurtarma olayları kapalı koşu: 1684.55 m, tepe noktası 19.347 s.

Tüm replay zamanlarında ateşleme öncesi 5 saniyelik kalibrasyon/bekleme bölümü vardır. Aşağıdaki gecikmeler tepe noktasına göredir.

| Senaryo | Beklenti | Sonuç | Tepe noktasından komuta gecikme |
|---|---|---|---|
| nominal | deploy | GEÇTİ | 0.713 s |
| no_noise | deploy | GEÇTİ | 0.733 s |
| gps_loss | deploy | GEÇTİ | 0.713 s |
| radio_stuck | deploy | GEÇTİ | 0.713 s |
| baro_spike | deploy | GEÇTİ | 0.713 s |
| baro_nan | deploy | GEÇTİ | 0.713 s |
| baro_gap_short | deploy | GEÇTİ | 0.713 s |
| baro_gap_apogee | deploy | GEÇTİ | 1.273 s |
| imu_gap_short | deploy | GEÇTİ | 0.713 s |
| baro_false_descent | deploy | GEÇTİ | 0.713 s |
| accel_bias_in_flight | deploy | GEÇTİ | 0.713 s |
| unarmed | inhibit | GEÇTİ | — |
| baro_dead | inhibit | GEÇTİ | — |
| baro_frozen | inhibit | GEÇTİ | — |
| imu_dead | inhibit | GEÇTİ | — |
| imu_gap_armed | inhibit | GEÇTİ | — |
| late_arm | inhibit | GEÇTİ | — |
| accel_saturated | inhibit | GEÇTİ | — |
| gyro_saturated | inhibit | GEÇTİ | — |
| boot_invalid | inhibit | GEÇTİ | — |
| arm_lost | inhibit | GEÇTİ | — |
| reset_in_flight | inhibit | GEÇTİ | — |
| clock_reversed | inhibit | GEÇTİ | — |
| pad_only | inhibit | GEÇTİ | — |
| pad_jolt | inhibit | GEÇTİ | — |
| monte_carlo_00 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_01 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_02 | deploy | GEÇTİ | 0.773 s |
| monte_carlo_03 | deploy | GEÇTİ | 0.733 s |
| monte_carlo_04 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_05 | deploy | GEÇTİ | 0.793 s |
| monte_carlo_06 | deploy | GEÇTİ | 0.733 s |
| monte_carlo_07 | deploy | GEÇTİ | 0.793 s |
| monte_carlo_08 | deploy | GEÇTİ | 0.833 s |
| monte_carlo_09 | deploy | GEÇTİ | 0.793 s |
| monte_carlo_10 | deploy | GEÇTİ | 0.733 s |
| monte_carlo_11 | deploy | GEÇTİ | 0.773 s |
| monte_carlo_12 | deploy | GEÇTİ | 0.733 s |
| monte_carlo_13 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_14 | deploy | GEÇTİ | 0.733 s |
| monte_carlo_15 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_16 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_17 | deploy | GEÇTİ | 0.793 s |
| monte_carlo_18 | deploy | GEÇTİ | 0.733 s |
| monte_carlo_19 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_20 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_21 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_22 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_23 | deploy | GEÇTİ | 0.833 s |
| monte_carlo_24 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_25 | deploy | GEÇTİ | 0.853 s |
| monte_carlo_26 | deploy | GEÇTİ | 0.773 s |
| monte_carlo_27 | deploy | GEÇTİ | 0.773 s |
| monte_carlo_28 | deploy | GEÇTİ | 0.753 s |
| monte_carlo_29 | deploy | GEÇTİ | 0.773 s |

## Sonucun anlamı

`inhibit` senaryolarının geçmesi, arızada kurtarma yapıldığı anlamına gelmez; yanlış/kanıtsız komut üretilmediğini gösterir. Kalıcı basınç/IMU arızası ve uçuşta reset bu özgün UKB’nin kurtarma görevini engeller. Ana Sky Logic V2 bu pakette modellenmedi.

30 Monte Carlo koşusu: sabit tohumlar, basınç/ivme/gyro gürültüsü, ±%1 ivme ölçeği, ±%0,5 gyro ölçeği ve 0–80 ms basınç gecikmesi. Bu aralıklar ölçülmüş hata dağılımları değildir.

Sanal sensörler analog bant genişliği, PCB, güç, montaj titreşimi, RF menzili ve elektromıknatısı doğrulamaz. OpenRocket yeniden örnekleme/enterpolasyon hatası vardır. Kurtarma açılması bu koşuya geri beslenmez. Komut sonrası yörünge gerçek iniş öngörüsü değildir.

ESP32 hedef derlemesi ayrıca `firmware-build.log` ile değerlendirilir; masaüstü testinin geçmesi hedef derlemesinin geçtiği anlamına gelmez. Resmî TEKNOFEST EK-7 protokolü bu sürümde uygulanmadı.
