#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// Nilai hasil kalibrasi dari Tabel L4
const float D_MIN_OPERASI_CM = 8.25f;
const float D_MAX_OPERASI_CM = 16.49f;

const float BATAS_ON_PCT = 70.0f;
const float BATAS_OFF_PCT = 60.0f;

const bool ENABLE_CLOUD = true; // Aktifkan koneksi WiFi & ThingSpeak
const char* WIFI_SSID = "ISI_SSID_WIFI";
const char* WIFI_PASS = "ISI_PASSWORD_WIFI";
unsigned long CHANNEL_ID = 0; // Isi dengan Channel ID ThingSpeak Anda
const char* WRITE_KEY = "ISI_WRITE_API_KEY";

const uint8_t OLED_ADDR = 0x3C; // sesuai scanner
const int PIN_PIR = D0, PIN_TRIG = D5, PIN_ECHO = D6;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
WiFiClient client;

bool oledOK = false, alarm = false, echoOK = false, inRange = false;
int pir = 0;
float jarak = NAN, kedekatan = NAN;
unsigned long lastRead = 0, lastSend = 0, lastWifi = 0;

float bacaJarakCM() {
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(12);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 30000UL);
  if (us == 0) return NAN;
  float d = us * 0.0343f / 2.0f;
  return (d >= 2 && d <= 400) ? d : NAN;
}

bool siapKalibrasi() {
  return D_MIN_OPERASI_CM >= 0.0f &&
         D_MAX_OPERASI_CM > D_MIN_OPERASI_CM;
}

void tampilLokal() {
  // Cek status koneksi WiFi
  String statusWifi;
  if (!ENABLE_CLOUD) {
    statusWifi = "OFF (LOKAL)";
  } else if (WiFi.status() == WL_CONNECTED) {
    statusWifi = "CONNECTED";
  } else {
    statusWifi = "CONNECTING...";
  }

  // Tampilan di Serial Monitor
  Serial.printf("WiFi=%s | PIR=%d | ", statusWifi.c_str(), pir);
  if (echoOK) Serial.printf("D=%.1f cm | ", jarak);
  else Serial.print("D=INVALID | ");
  if (inRange) Serial.printf("K=%.1f%%", kedekatan);
  else Serial.print("K=INVALID/OUTRANGE");
  Serial.printf(" | ALARM=%d\n", alarm ? 1 : 0);

  // Tampilan di Layar OLED
  if (!oledOK) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  
  oled.println("W13 - KALIBRASI");
  oled.print("WiFi : "); oled.println(statusWifi);
  oled.print("PIR  : "); oled.println(pir);
  
  if (echoOK) {
    oled.print("Jarak: "); oled.print(jarak, 1);
    oled.println(" cm");
  } else {
    oled.println("Jarak: INVALID");
  }

  if (inRange) {
    oled.print("Dekat: "); oled.print(kedekatan, 1);
    oled.println(" %");
  } else {
    oled.println("Dekat: INVALID");
  }

  oled.print("ALARM: "); oled.println(alarm ? "ON" : "OFF");
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Wire.begin(D2, D1);
  oledOK = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  if (!siapKalibrasi())
    Serial.println("ISI D_MIN / D_MAX DARI TABEL L4");

  if (ENABLE_CLOUD) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    ThingSpeak.begin(client);
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastRead >= 1000UL) {
    lastRead = now;
    pir = digitalRead(PIN_PIR);
    jarak = bacaJarakCM();
    echoOK = isfinite(jarak);
    inRange = siapKalibrasi() && echoOK &&
              jarak >= D_MIN_OPERASI_CM &&
              jarak <= D_MAX_OPERASI_CM;
    kedekatan = inRange ?
                100.0f * (D_MAX_OPERASI_CM - jarak) /
                (D_MAX_OPERASI_CM - D_MIN_OPERASI_CM) : NAN;

    bool pirSiap = (now >= 60000UL);
    if (!pirSiap || pir == LOW || !inRange) alarm = false;
    else if (!alarm && kedekatan >= BATAS_ON_PCT)
      alarm = true;
    else if (alarm && kedekatan <= BATAS_OFF_PCT)
      alarm = false;

    tampilLokal();
  }

  if (!ENABLE_CLOUD) return;

  if (WiFi.status() != WL_CONNECTED &&
      now - lastWifi >= 12000UL) {
    lastWifi = now;
    WiFi.reconnect();
  }

  if (now - lastSend >= 20000UL) {
    lastSend = now;
    if (WiFi.status() == WL_CONNECTED && CHANNEL_ID) {
      // String kosong mencegah data lama ikut terkirim.
      ThingSpeak.setField(1, echoOK ? String(jarak, 1) : String(""));
      ThingSpeak.setField(2, pir);
      ThingSpeak.setField(3, alarm ? 1 : 0);
      ThingSpeak.setField(4, inRange ? String(kedekatan, 1) : String(""));
      ThingSpeak.setField(5, inRange ? 1 : 0);
      int rc = ThingSpeak.writeFields(CHANNEL_ID, WRITE_KEY);
      Serial.printf("HTTP ThingSpeak = %d\n", rc);
    } else {
      Serial.println("OFFLINE: lokal tetap bekerja");
    }
  }
}