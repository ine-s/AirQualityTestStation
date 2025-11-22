#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>

// === Configuration ===
#define PIN_NEOPIX 17
#define NPIX 12
#define SEALEVEL_HPA 1013.25

Adafruit_BME280 bme;
Adafruit_NeoPixel strip(NPIX, PIN_NEOPIX, NEO_GRB + NEO_KHZ800);

// --- PMS7003 sur Serial1 (Nano Every) ---
struct PMSData {
  uint16_t pm1_0;
  uint16_t pm2_5;
  uint16_t pm10;
};
PMSData pms;

// === Lecture du PMS7003 ===
bool readPMS7003(PMSData &data) {
  static uint8_t buffer[32];
  static uint8_t index = 0;

  while (Serial1.available()) {
    uint8_t byteIn = Serial1.read();

    if (index == 0 && byteIn != 0x42) continue;
    if (index == 1 && byteIn != 0x4D) {
      index = 0;
      continue;
    }

    buffer[index++] = byteIn;

    if (index >= 32) {
      index = 0;
      data.pm1_0 = (buffer[10] << 8) | buffer[11];
      data.pm2_5 = (buffer[12] << 8) | buffer[13];
      data.pm10  = (buffer[14] << 8) | buffer[15];
      return true;
    }
  }
  return false;
}

// === Calibration conditionnelle en fonction T° / HR (selon doc PMS7003) ===
float corrigerPM25(float pm25, float temp, float hum) {
  float pm_corr = pm25;

  // --- Correction humidité ---
  if (hum > 70) {
    pm_corr *= (1 - 0.002 * (hum - 70));
  } 
  else if (hum < 30) {
    pm_corr *= (1 + 0.002 * (30 - hum));
  }

  // --- Correction température ---
  if (temp > 35) {
    pm_corr *= (1 - 0.01 * (temp - 35));
  } 
  else if (temp < 10) {
    pm_corr *= (1 + 0.01 * (10 - temp));
  }

  return pm_corr;
}

// === Couleur selon PM2.5 corrigé ===
uint32_t pmToColor(float pm25) {
  if (pm25 <= 12) return strip.Color(0, 255, 0);      // Vert
  if (pm25 <= 35) return strip.Color(255, 255, 0);    // Jaune
  if (pm25 <= 55) return strip.Color(255, 140, 0);    // Orange
  if (pm25 <= 150) return strip.Color(255, 0, 0);     // Rouge
  return strip.Color(180, 0, 255);                    // Violet
}

// === Nombre de LEDs selon la température ===
int ledsForTemp(float t) {
  if (t <= 10) return 2;
  if (t <= 15) return 4;
  if (t <= 20) return 6;
  if (t <= 25) return 8;
  if (t <= 30) return 10;
  return 12;
}

// === Animation ===
void breathingEffect(uint32_t color) {
  for (int b = 10; b <= 180; b += 5) {   // montée 
    strip.setBrightness(b);
    strip.fill(color);
    strip.show();
    delay(30);
  }
  for (int b = 180; b >= 10; b -= 5) {   // descente 
    strip.setBrightness(b);
    strip.fill(color);
    strip.show();
    delay(30);
  }
}

// === Affiche couleur + nombre de LEDs ===
void showTempAirVisual(float t, float pm25corr, float hum, bool unstable) {
  uint32_t col = pmToColor(pm25corr);
  int n = ledsForTemp(t);

  // Luminosité en fonction de l’humidité (air humide = plus faible)
  int bright = map((int)hum, 20, 90, 255, 60);
  strip.setBrightness(constrain(bright, 60, 255));

  for (int i = 0; i < NPIX; i++) {
    strip.setPixelColor(i, (i < n) ? col : strip.Color(0, 0, 0));
  }
  strip.show();

  // Si variation importante : effet de respiration
  if (unstable) breathingEffect(col);
}

// === Initialisation ===
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(1000);

  strip.begin();
  strip.setBrightness(70);
  strip.show();

  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("BME280 non trouvé !");
    for (int i = 0; i < NPIX; i++) strip.setPixelColor(i, strip.Color(255, 0, 0));
    strip.show();
    while (1);
  }
}

// === Boucle principale ===
void loop() {
  static unsigned long lastDisplay = 0;
  static bool hasData = false;
  static float lastPM = 0;

  if (readPMS7003(pms)) hasData = true;

  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0F;

  if (millis() - lastDisplay > 3000) {
    lastDisplay = millis();

    Serial.println("─────────────── [STATION AIR MONITOR] ───────────────");
    Serial.print("Température : "); Serial.print(t, 1); Serial.println(" °C");
    Serial.print("Humidité    : "); Serial.print(h, 1); Serial.println(" %");
    Serial.print("Pression    : "); Serial.print(p, 1); Serial.println(" hPa");

    if (hasData) {
      float pm_corr = corrigerPM25(pms.pm2_5, t, h);
      bool unstable = abs(pm_corr - lastPM) > 5; // si variation > 5 µg/m³
      lastPM = pm_corr;

      Serial.println("\nPMS7003 :");
      Serial.print("   - PM1.0 : "); Serial.print(pms.pm1_0); Serial.println(" µg/m³");
      Serial.print("   - PM2.5 : "); Serial.print(pms.pm2_5);
      Serial.print(" µg/m³  →  "); Serial.print(pm_corr, 1); Serial.println(" µg/m³ corrigé selon T° et   HR)");
      Serial.print("   - PM10  : "); Serial.print(pms.pm10); Serial.println(" µg/m³");

      Serial.print("\n Interprétation : ");
      if (pm_corr <= 12) Serial.println("Air excellent");
      else if (pm_corr <= 35) Serial.println("Air correct");
      else if (pm_corr <= 55) Serial.println("Air pollué");
      else if (pm_corr <= 150) Serial.println("Air mauvais");
      else Serial.println("Air dangereux");

      showTempAirVisual(t, pm_corr, h, unstable);
      hasData = false;
    } else {
      Serial.println("\n PMS7003 : aucune donnée reçue !");
    }
    Serial.println("─────────────────────────────────────────────────────\n");
  }
}

