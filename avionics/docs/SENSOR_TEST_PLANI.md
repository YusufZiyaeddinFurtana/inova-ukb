# Sensör uyarımıyla SİT/SUT test sınırları

Kullanıcının 17 Ekim 2026 isteği: EK-7 testlerini koda uygulamak ve sensör verisini elde etmek için sensörü simülasyonda uyarmak. Bu kapsamda aşağıdaki iki arayüz sınanır:

1. **Fiziksel uyarım → sanal sensör/I²C → gerçek sürücü → SİT paketi.** İvme, dönme, yükseklik/basınç ve sıcaklık değişimleri ham register değerlerine dönüşür. MPU9255 sürücüsü ile Adafruit BMP3XX/Bosch ve BusIO kaynakları aynen derlenir. SİT ölçüm/eksen/filtre/zamanlama davranışını yakalar; gerçek silisyumu, elektriksel I²C veya ESP32 zamanlayıcısını kanıtlamaz.
2. **SUT komutu/veri paketleri → aynı uçuş algoritması → durum/ayrılma isteği.** Normal/anormal uçuşlar, gerçek sensörlerin uyarılmasına rağmen sentetik kaynağın tek karar girişi olması ve test yaşam döngüsü sınanır. Gerçek kurtarma sürücüsü, RS232 gerilimleri ve hakem cihazı bu sınırın dışındadır.

Testler yeni bir sanal sensör özelliği veya ölçüm davranışı eklendikçe birer dikey adımla yazılır ve çalıştırılır. Karar eşikleri testleri geçirmek amacıyla gevşetilmez. Gürültü/filtre ve kalibrasyon modeli varsayımları sonuç raporunda görünür tutulur. Başarılı mantıksal istek, fiziksel paraşüt açılması değildir.
