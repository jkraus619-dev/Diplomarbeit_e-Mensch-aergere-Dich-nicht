#ifndef MICROSDCARD_H
#define MICROSDCARD_H

#include <Arduino.h>

// SPI-Pins fuer die SD-Karte
#define SD_CS_PIN  20
#define SD_SCK_PIN  8
#define SD_MISO_PIN 18
#define SD_MOSI_PIN 19

// Initialisiert die SD-Karte einmalig. Muss in setupAll() nach SPI.begin()
// und VOR lcdDiceInit() aufgerufen werden.
bool microsdcard_init();

// Prueft Benutzername und Passwort gegen benutzer.csv.
// Rueckgabe: true wenn Kombination korrekt ist.
bool microsdcard_user_check_password(const String& name, const String& password);

// Prueft ob ein Benutzer bereits in benutzer.csv eingetragen ist.
bool microsdcard_user_exists(const String& name);

// Haengt einen neuen Benutzer ans Ende von benutzer.csv an (Gewinne/Spiele = 0).
// Erstellt die Datei inkl. Header wenn sie noch nicht existiert.
bool microsdcard_user_append(const String& name, const String& password);

// Schreibt Dummy-Benutzerdaten (CSV) und Batterie-Log auf die SD-Karte
// und liest sie anschliessend wieder aus.
// Rueckgabe: true wenn beide Dateien erfolgreich geschrieben und gelesen wurden.
bool microsdcard_dummy_write_and_read(String& usersCsvOut, String& batteryLogOut);

// Haengt einen Batterie-Messwert mit Timestamp an /battery.csv an.
// Erstellt die Datei inkl. Header wenn sie noch nicht existiert.
// Tut nichts wenn die SD-Karte nicht bereit ist.
void sdcard_logBatteryVoltage(float voltage);

// Liest /battery.csv und gibt die letzten 500 Eintraege als JSON-Array zurueck.
// Format: [{"t":1700000000,"v":3.72}, ...]
// Gibt "[]" zurueck bei Fehler oder leerer Datei.
String sdcard_getBatteryJson();

#endif // MICROSDCARD_H
