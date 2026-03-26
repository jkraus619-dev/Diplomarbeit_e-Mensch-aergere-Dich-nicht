#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Pins am ESP32-Board für Batterie-Messung und LED-Streifen
#ifndef BATTERY_ADC_PIN
#define BATTERY_ADC_PIN 9   // ADC1 pin for voltage divider
#endif

//!muss noch angepasst werden wenn das PCB fertig ist!
#ifndef BATTERY_LED_PIN
#define BATTERY_LED_PIN 21   // Data pin for WS2812B
#endif

#ifndef BATTERY_LED_COUNT
#define BATTERY_LED_COUNT 10 // Number of segments
#endif

// Initialisiert ADC und LED-Streifen
void batteryInit();
// liest die Batteriespannung aus
float batteryReadVoltage();
// Spannung in Ladezustand (0..100%) umrechnen
int batteryPercent(float vbat);
// updated LED-Streifen entsprechend Ladezustand
void batteryLedsShowPercent(int percent);
