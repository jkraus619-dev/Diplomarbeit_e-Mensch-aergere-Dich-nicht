#include "battery.h"

static Adafruit_NeoPixel batstrip(BATTERY_LED_COUNT, BATTERY_LED_PIN, NEO_GRB + NEO_KHZ800);

static constexpr float R1 = 10000.0f;
static constexpr float R2 = 10000.0f;
static constexpr int ADC_MAX = 4095;
static constexpr float VREF = 3.30f;
static constexpr int SAMPLE_COUNT = 64;
static constexpr int SAMPLE_DELAY = 150;
static float VREF_CAL = 1.05f;

// Liest ADC-Werte und filtert Ausreißer (min/max entfernen)
static float readFilteredAdc() {
  uint32_t acc = 0;
  uint16_t minv = 0xFFFF;
  uint16_t maxv = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    uint16_t s = analogRead(BATTERY_ADC_PIN);
    acc += s;
    if (s < minv) minv = s;
    if (s > maxv) maxv = s;
    delayMicroseconds(SAMPLE_DELAY);
  }
  return (acc - minv - maxv) / float(SAMPLE_COUNT - 2);
}

// Initialisiert ADC und LED-Streifen
void batteryInit() {
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  pinMode(BATTERY_ADC_PIN, INPUT);

  //batstrip.begin();
  //batstrip.setBrightness(64);
  //batstrip.show();
}

// liest die Batteriespannung aus
float batteryReadVoltage() {
  float adc = readFilteredAdc();
  float spg_adcpin = (adc / ADC_MAX) * VREF * VREF_CAL;
  return spg_adcpin * (R1 + R2) / R2;
}

// Spannung in Ladezustand (0..100%) umrechnen
int batteryPercent(float v) {
  struct P { float volt; int percent; } pts[] = {
    {3.00f,   0}, 
    {3.10f,   8},
    {3.20f,  17},
    {3.30f,  25},
    {3.40f,  33}, 
    {3.50f,  42},
    {3.60f,  50},
    {3.70f,  58},
    {3.80f,  67},
    {3.90f,  75},
    {4.00f,  83},
    {4.10f,  92},
    {4.20f, 100}
  };
  constexpr int N = sizeof(pts) / sizeof(pts[0]);
  if (v <= pts[0].volt) return 0;
  if (v >= pts[N - 1].volt) return 100;
  for (int i = 0; i < N - 1; ++i) {
    if (v >= pts[i].volt && v <= pts[i + 1].volt) {
      float t = (v - pts[i].volt) / (pts[i + 1].volt - pts[i].volt);
      return int(pts[i].percent + t * (pts[i + 1].percent - pts[i].percent) + 0.5f);
    }
  }
  return 0;
}

// updated LED-Streifen entsprechend Ladezustand
void batteryLedsShowPercent(int p) {
  auto col = [](uint8_t r, uint8_t g, uint8_t b){ return batstrip.Color(r, g, b); };

  int clamped = constrain(p, 0, 100);
  int seg = (clamped + 9) / 10; // 0..10 segments

  for (int i = 0; i < BATTERY_LED_COUNT; ++i) {
    uint32_t c = col(0, 0, 0);
    if (i < seg) {
      if (i <= 2)      c = col(200, 20, 20);  // red
      else if (i <= 6) c = col(220, 150, 10); // yellow
      else             c = col(20, 180, 40);  // green
    }
    batstrip.setPixelColor(i, c);
  }

  if (clamped > 0 && clamped < 10) {
    static bool t = false;
    t = !t;
    batstrip.setPixelColor(0, t ? col(255, 30, 30) : col(100, 0, 0));
  }

  batstrip.show();
}
