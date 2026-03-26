#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>
#include "microsdcard.h"

// SD-Karte bleibt nach microsdcard_init() dauerhaft gemountet.
// SD.begin() / SD.end() werden nirgendwo sonst aufgerufen.
// Globale SPI-Instanz – einmalig mit korrekten Pins konfiguriert.
// lcdDiceInit() darf SPI.begin() NICHT nochmal aufrufen.
static bool sdReady = false;

static const char* USERS_PATH   = "/benutzer.csv";
static const char* BATTERY_PATH = "/batterie.log";

// ---------------------------------------------------------------------------
// Hilfsfunktionen (intern)
// ---------------------------------------------------------------------------
static String buildUserLine(const String& name, const String& password) {
  return name + "," + password + ",0,0,0\n";
}

// ---------------------------------------------------------------------------
// Oeffentliche Funktion: SD-Karte einmalig initialisieren
// Muss in setupAll() nach SPI.begin() und VOR lcdDiceInit() aufgerufen werden.
// ---------------------------------------------------------------------------
bool microsdcard_init() {
  // SPI einmalig mit korrekten Pins initialisieren (MISO muss gesetzt bleiben!)
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  sdReady = SD.begin(SD_CS_PIN, SPI, 4000000);
  if (sdReady) {
    Serial.printf("[SD] Initialisiert. Typ: %d | Groesse: %llu MB\n",
                  SD.cardType(), SD.cardSize() / (1024ULL * 1024ULL));
  } else {
    Serial.println("[SD] FEHLER: SD.begin() fehlgeschlagen");
  }
  return sdReady;
}

// ---------------------------------------------------------------------------
// Oeffentliche Funktion: Prueft Benutzername + Passwort gegen die CSV
// ---------------------------------------------------------------------------
bool microsdcard_user_check_password(const String& name, const String& password) {
  Serial.printf("[SD] Login-Versuch: '%s'\n", name.c_str());

  if (!sdReady) {
    Serial.println("[SD] FEHLER: SD nicht initialisiert");
    return false;
  }

  File f = SD.open(USERS_PATH, FILE_READ);
  if (!f) {
    Serial.println("[SD] FEHLER: benutzer.csv nicht geoeffnet");
    return false;
  }

  bool ok = false;
  bool firstLine = true;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (firstLine) { firstLine = false; continue; }
    if (!line.length()) continue;

    int c1 = line.indexOf(',');
    if (c1 < 0) continue;
    int c2 = line.indexOf(',', c1 + 1);
    if (c2 < 0) continue;

    String csvName = line.substring(0, c1);
    String csvPass = line.substring(c1 + 1, c2);

    Serial.printf("[SD]   Vergleiche: '%s' / '%s'\n", csvName.c_str(), csvPass.c_str());

    if (csvName.equalsIgnoreCase(name) && csvPass == password) {
      ok = true;
      break;
    }
  }
  f.close();
  Serial.printf("[SD] Ergebnis: %s\n", ok ? "OK" : "fehlgeschlagen");
  return ok;
}

// ---------------------------------------------------------------------------
// Oeffentliche Funktion: Prueft ob ein Benutzer bereits in der CSV steht
// ---------------------------------------------------------------------------
bool microsdcard_user_exists(const String& name) {
  if (!sdReady) return false;

  File f = SD.open(USERS_PATH, FILE_READ);
  if (!f) return false;

  bool found = false;
  bool firstLine = true;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (firstLine) { firstLine = false; continue; }
    if (!line.length()) continue;
    int comma = line.indexOf(',');
    String csvName = (comma >= 0) ? line.substring(0, comma) : line;
    if (csvName.equalsIgnoreCase(name)) {
      found = true;
      break;
    }
  }
  f.close();
  return found;
}

// ---------------------------------------------------------------------------
// Oeffentliche Funktion: Neuen Benutzer an die CSV anhaengen
// ---------------------------------------------------------------------------
bool microsdcard_user_append(const String& name, const String& password) {
  if (!sdReady) return false;

  // Header schreiben falls Datei noch nicht existiert
  if (!SD.exists(USERS_PATH)) {
    File hdr = SD.open(USERS_PATH, FILE_WRITE);
    if (!hdr) return false;
    hdr.print("Benutzername,Passwort,Gewinne,Insgesamt Spiele,6er gewuerfelt\n");
    hdr.close();
  }

  File f = SD.open(USERS_PATH, FILE_APPEND);
  if (!f) return false;
  f.print(buildUserLine(name, password));
  f.close();
  return true;
}

// ---------------------------------------------------------------------------
// Oeffentliche Funktion: Dummy-Daten schreiben und zuruecklesen (nur fuer Tests)
// ---------------------------------------------------------------------------
bool microsdcard_dummy_write_and_read(String& usersCsvOut, String& batteryLogOut) {
  if (!sdReady) return false;

  // Dummy-Inhalt aufbauen
  usersCsvOut  = "Benutzername,Passwort,Gewinne,Insgesamt Spiele,6er gewuerfelt\n";
  usersCsvOut += "Anna,pass123,5,12,3\nBen,qwerty,2,7,1\nClara,abc123,9,15,4\n";
  batteryLogOut  = "2026-02-18 09:00:00,4.12V\n";
  batteryLogOut += "2026-02-18 09:30:00,4.06V\n";
  batteryLogOut += "2026-02-18 10:00:00,3.98V\n";

  // Schreiben
  File fw = SD.open(USERS_PATH, FILE_WRITE);
  if (!fw) return false;
  fw.print(usersCsvOut);
  fw.close();

  File fb = SD.open(BATTERY_PATH, FILE_WRITE);
  if (!fb) return false;
  fb.print(batteryLogOut);
  fb.close();

  // Zuruecklesen zur Verifikation
  File fr = SD.open(USERS_PATH, FILE_READ);
  if (fr) { usersCsvOut = fr.readString(); fr.close(); }

  File fbr = SD.open(BATTERY_PATH, FILE_READ);
  if (fbr) { batteryLogOut = fbr.readString(); fbr.close(); }

  return true;
}

// ---------------------------------------------------------------------------
// Oeffentliche Funktion: Batteriespannung mit Timestamp an /battery.csv anhaengen
// ---------------------------------------------------------------------------
void sdcard_logBatteryVoltage(float voltage) {
  if (!sdReady) return;

  static const char* CSV_PATH = "/battery.csv";

  if (!SD.exists(CSV_PATH)) {
    File hdr = SD.open(CSV_PATH, FILE_WRITE);
    if (!hdr) return;
    hdr.print("timestamp,voltage\n");
    hdr.close();
  }

  File f = SD.open(CSV_PATH, FILE_APPEND);
  if (!f) return;
  unsigned long ts = (unsigned long)time(nullptr);
  f.printf("%lu,%.2f\n", ts, voltage);
  f.close();
}

// ---------------------------------------------------------------------------
// Oeffentliche Funktion: /battery.csv einlesen, letzte 500 Eintraege als JSON
// ---------------------------------------------------------------------------
String sdcard_getBatteryJson() {
  if (!sdReady) return "[]";

  static const char* CSV_PATH = "/battery.csv";
  if (!SD.exists(CSV_PATH)) return "[]";

  // Pass 1: gueltige Zeilen zaehlen
  File f = SD.open(CSV_PATH, FILE_READ);
  if (!f) return "[]";
  int lineCount = 0;
  bool firstLine = true;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (firstLine) { firstLine = false; continue; }
    if (line.length() > 0 && line.indexOf(',') >= 0) lineCount++;
  }
  f.close();

  if (lineCount == 0) return "[]";

  // Pass 2: ggf. aeltere Zeilen ueberspringen, JSON aufbauen
  int skip = (lineCount > 500) ? lineCount - 500 : 0;
  f = SD.open(CSV_PATH, FILE_READ);
  if (!f) return "[]";

  String json = "[";
  bool first = true;
  firstLine = true;
  int skipped = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (firstLine) { firstLine = false; continue; }
    if (line.length() == 0) continue;
    int comma = line.indexOf(',');
    if (comma < 0) continue;
    if (skipped < skip) { skipped++; continue; }
    String tsStr = line.substring(0, comma);
    String vStr  = line.substring(comma + 1);
    if (!first) json += ",";
    json += "{\"t\":" + tsStr + ",\"v\":" + vStr + "}";
    first = false;
  }
  f.close();
  json += "]";
  return json;
}
