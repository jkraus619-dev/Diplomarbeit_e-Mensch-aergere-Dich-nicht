#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

/*
#include <SPI.h>
#include <SD.h>

#define SD_CS   10
#define SD_SCK  12
#define SD_MISO 13
#define SD_MOSI 11

void setup() {
  Serial.begin(115200);
  delay(300);

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD mount FAIL");
    return;
  }
  Serial.println("SD OK");

  // ---- schreiben (überschreiben) ----
  File w = SD.open("/test.txt", FILE_WRITE);
  if (!w) { Serial.println("open write FAIL"); return; }
  w.println("Hallo SD Karte!");
  w.close();
  Serial.println("write OK");

  // ---- lesen ----
  File r = SD.open("/test.txt");
  if (!r) { Serial.println("open read FAIL"); return; }
  Serial.println("---- Dateiinhalt ----");
  while (r.available()) Serial.write(r.read());
  r.close();
  Serial.println("\n---------------------");
}

void loop() {}
*/

namespace {
static constexpr int SD_CS_PIN = SS;
static const char* USERS_PATH = "/benutzer.csv";
static const char* BATTERY_PATH = "/batterie.log";

struct DummyUser {
  const char* name;
  const char* password;
  int wins;
  int totalGames;
  int sixes;
};

static const DummyUser kDummyUsers[] = {
  {"Anna",  "pass123", 5, 12, 3},
  {"Ben",   "qwerty",  2,  7, 1},
  {"Clara", "abc123",  9, 15, 4}
};

struct DummyBatterySample {
  const char* time;
  float voltage;
};

static const DummyBatterySample kBatterySamples[] = {
  {"2026-02-18 09:00:00", 4.12f},
  {"2026-02-18 09:30:00", 4.06f},
  {"2026-02-18 10:00:00", 3.98f}
};

String buildUsersCsv() {
  String s;
  s.reserve(256);
  s += "Benutzername,Passwort,Gewinne,Insgesamt Spiele,6er gewuerfelt\n";
  for (const auto& u : kDummyUsers) {
    s += u.name;
    s += ',';
    s += u.password;
    s += ',';
    s += String(u.wins);
    s += ',';
    s += String(u.totalGames);
    s += ',';
    s += String(u.sixes);
    s += '\n';
  }
  return s;
}

String buildBatteryLog() {
  String s;
  s.reserve(128);
  for (const auto& b : kBatterySamples) {
    s += b.time;
    s += ',';
    s += String(b.voltage, 2);
    s += "V\n";
  }
  return s;
}

bool writeTextFile(fs::FS& fs, const char* path, const String& content) {
  File f = fs.open(path, FILE_WRITE);
  if (!f) {
    return false;
  }
  size_t n = f.print(content);
  f.close();
  return n == content.length();
}

String readTextFile(fs::FS& fs, const char* path) {
  File f = fs.open(path, FILE_READ);
  if (!f) {
    return String();
  }
  String out = f.readString();
  f.close();
  return out;
}
} // namespace

// Schreibt Dummy-Daten auf die SD und gibt die Inhalte als String zurueck.
// Rueckgabe: true wenn beide Dateien geschrieben und gelesen wurden.
bool microsdcard_dummy_write_and_read(String& usersCsvOut, String& batteryLogOut) {
  usersCsvOut = buildUsersCsv();
  batteryLogOut = buildBatteryLog();

  if (!SD.begin(SD_CS_PIN)) {
    return false;
  }

  bool okUsers = writeTextFile(SD, USERS_PATH, usersCsvOut);
  bool okBatt = writeTextFile(SD, BATTERY_PATH, batteryLogOut);

  if (okUsers) {
    usersCsvOut = readTextFile(SD, USERS_PATH);
  }
  if (okBatt) {
    batteryLogOut = readTextFile(SD, BATTERY_PATH);
  }

  SD.end();
  return okUsers && okBatt;
}
