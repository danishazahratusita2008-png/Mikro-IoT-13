# Sistem Deteksi Kedekatan & Gerakan Berbasis ESP8266 (PIR + HC-SR04)

Proyek praktikum IoT menggunakan NodeMCU ESP8266, sensor PIR, sensor ultrasonik HC-SR04, layar OLED 0x3C, dan integrasi cloud ThingSpeak.

## Parameter Kalibrasi (Tabel L4)
- **Batas Jarak Minimum Bersama (D_min):** 8.25 cm
- **Batas Jarak Maksimum Bersama (D_maks):** 16.49 cm
- **Ambang Alarm ON:** Kedekatan >= 70% (dan PIR terdeteksi setelah 60 detik stabilisasi)
- **Ambang Alarm OFF:** Kedekatan <= 60%

## Pemetaan Pin NodeMCU ESP8266
| Komponen | Pin Modul | Pin NodeMCU |
| :--- | :---: | :---: |
| OLED (0x3C) | SDA / SCL | D2 / D1 |
| Sensor PIR | OUT | D0 |
| HC-SR04 | TRIG / ECHO | D5 / D6 |

## Pemetaan Field ThingSpeak
1. **Field 1:** Jarak (cm)
2. **Field 2:** PIR (0/1)
3. **Field 3:** Alarm (0/1)
4. **Field 4:** Kedekatan (%)
5. **Field 5:** In Range (0/1)
