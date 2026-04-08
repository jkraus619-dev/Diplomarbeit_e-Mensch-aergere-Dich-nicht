#include "wifi_ap.h"
#include <WiFi.h>
#include <esp_log.h>
#include <SPI.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "battery.h"
#include "lcd_dice.h"
#include "hardware.h"
#include "microsdcard.h"

bool gameStarted = false; // Flag initial false
bool rolled = false;      // Flag initial false
int auswahl = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char *ssid = "LudoBoard";
const char *password = "ludo1234";

static unsigned long lastBatMs = 0;
static bool batterySendRequested = false;

static bool changetosleep = false;

// Forward declarations
void broadcastJson(const JsonDocument &doc);
void setupPeriphery();

// ------------------- Lobby State -------------------
static const int MAX_LOBBY_PLAYERS = 4;
struct LobbyPlayer
{
  String name;
  String color;
};

static LobbyPlayer lobbyPlayers[MAX_LOBBY_PLAYERS];
static int lobbyPlayerCount = 0;

static int findLobbyIndex(const String &name)
{
  for (int i = 0; i < lobbyPlayerCount; i++)
  {
    if (lobbyPlayers[i].name == name)
    {
      return i;
    }
  }
  return -1;
}

static void clearLobbyState()
{
  lobbyPlayerCount = 0;
  for (int i = 0; i < MAX_LOBBY_PLAYERS; i++)
  {
    lobbyPlayers[i].name = "";
    lobbyPlayers[i].color = "";
  }
}

static bool addLobbyPlayer(const String &name)
{
  if (!name.length())
  {
    return false;
  }
  if (findLobbyIndex(name) >= 0)
  {
    return true;
  }
  if (lobbyPlayerCount >= MAX_LOBBY_PLAYERS)
  {
    return false;
  }
  lobbyPlayers[lobbyPlayerCount].name = name;
  lobbyPlayers[lobbyPlayerCount].color = "";
  lobbyPlayerCount += 1;
  return true;
}

static void removeLobbyPlayerByName(const String &name)
{
  int index = findLobbyIndex(name);
  if (index < 0)
  {
    return;
  }
  for (int i = index; i < lobbyPlayerCount - 1; i++)
  {
    lobbyPlayers[i] = lobbyPlayers[i + 1];
  }
  lobbyPlayerCount -= 1;
  if (lobbyPlayerCount < 0)
  {
    lobbyPlayerCount = 0;
  }
}

static void setLobbyColor(const String &name, const String &color)
{
  int index = findLobbyIndex(name);
  if (index < 0)
  {
    return;
  }
  lobbyPlayers[index].color = color;
}

static void sendLobbySnapshotToAll()
{
  StaticJsonDocument<256> doc;
  doc["type"] = "lobby";
  JsonArray players = doc.createNestedArray("players");
  for (int i = 0; i < lobbyPlayerCount; i++)
  {
    JsonObject entry = players.createNestedObject();
    entry["name"] = lobbyPlayers[i].name;
    entry["color"] = lobbyPlayers[i].color;
  }
  broadcastJson(doc);
}

static void sendLobbySnapshotToClient(AsyncWebSocketClient *client)
{
  if (!client)
  {
    return;
  }
  StaticJsonDocument<256> doc;
  doc["type"] = "lobby";
  JsonArray players = doc.createNestedArray("players");
  for (int i = 0; i < lobbyPlayerCount; i++)
  {
    JsonObject entry = players.createNestedObject();
    entry["name"] = lobbyPlayers[i].name;
    entry["color"] = lobbyPlayers[i].color;
  }
  String out;
  serializeJson(doc, out);
  client->text(out);
}

// ------------------- JSON Broadcast -------------------
void broadcastJson(const JsonDocument &doc)
{
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

// ------------------- Akku-Status senden -------------------
static void sendBatterySnapshot()
{
  float v = batteryReadVoltage();
  int pct = batteryPercent(v);
  if (!spielRunning)
  {
    lcdDiceShowBattery(v, pct);
  }
  sdcard_logBatteryVoltage(v);
  Serial.printf("Battery: %.2f V (%d%%)\n", v, pct);

  StaticJsonDocument<192> doc;
  doc["type"] = "battery";
  doc["mv"] = int(v * 1000 + 0.5f);
  doc["percent"] = pct;
  doc["ts"] = (uint32_t)millis();
  broadcastJson(doc);
}

// ------------------- WebSocket Nachrichten -------------------
void handleWsMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->opcode != WS_TEXT)
    return;

  // -------- String extrahieren --------
  String msg;
  for (size_t i = 0; i < len; i++)
    msg += (char)data[i];
  msg.trim();

  Serial.printf("WS received: %s\n", msg.c_str());

  // ======================================================
  // 1) JSON erkannt
  // ======================================================
  if (msg.startsWith("{") && msg.endsWith("}"))
  {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, msg);

    if (err)
    {
      Serial.printf("WS JSON-Error: %s\n", err.c_str());

      StaticJsonDocument<128> out;
      out["type"] = "error";
      out["msg"] = "invalid json";
      broadcastJson(out);
      return;
    }

    // JSON muss ein cmd enthalten
    if (!doc.containsKey("cmd"))
    {
      StaticJsonDocument<128> out;
      out["type"] = "error";
      out["msg"] = "json missing cmd";
      broadcastJson(out);
      return;
    }

    String cmd = doc["cmd"].as<String>();
    Serial.printf("WS JSON command: %s\n", cmd.c_str());

    // ------------ JSON-Befehle ----------
    if (cmd == "login")
    {
      String user = doc["user"] | "";
      String pass = doc["password"] | "";

      Serial.printf("[LOGIN] Handler erreicht. user='%s' pass='%s'\n", user.c_str(), pass.c_str());

      StaticJsonDocument<128> out;
      if (!user.length() || !pass.length())
      {
        out["type"] = "loginErr";
        out["msg"] = "Fehlende Felder";
      }
      else if (microsdcard_user_check_password(user, pass))
      {
        Serial.printf("[SD] Login OK: %s\n", user.c_str());
        out["type"] = "loginOk";
        out["user"] = user;
      }
      else
      {
        Serial.printf("[SD] Login fehlgeschlagen: %s\n", user.c_str());
        out["type"] = "loginErr";
        out["msg"] = "Benutzername oder Passwort falsch";
      }
      broadcastJson(out);
      return;
    }

    if (cmd == "register")
    {
      String user = doc["user"] | "";
      String pass = doc["password"] | "";

      StaticJsonDocument<128> out;
      if (!user.length() || !pass.length())
      {
        out["type"] = "registerErr";
        out["msg"] = "Fehlende Felder";
      }
      else if (microsdcard_user_exists(user))
      {
        out["type"] = "registerErr";
        out["msg"] = "Benutzer existiert bereits";
      }
      else if (microsdcard_user_append(user, pass))
      {
        Serial.printf("[SD] Neuer Benutzer gespeichert: %s\n", user.c_str());
        out["type"] = "registerOk";
        out["user"] = user;
      }
      else
      {
        out["type"] = "registerErr";
        out["msg"] = "SD-Fehler beim Speichern";
      }
      broadcastJson(out);
      return;
    }

    if (cmd == "lobby")
    {
      String action = doc["action"] | "";
      String user = doc["user"] | "";
      String color = doc["color"] | "";

      if (action == "create")
      {
        clearLobbyState();
        addLobbyPlayer(user);
        sendLobbySnapshotToAll();
        return;
      }
      if (action == "join")
      {
        addLobbyPlayer(user);
        sendLobbySnapshotToAll();
        return;
      }
      if (action == "remove")
      {
        removeLobbyPlayerByName(user);
        sendLobbySnapshotToAll();
        return;
      }
      if (action == "setColor")
      {
        setLobbyColor(user, color);
        sendLobbySnapshotToAll();
        return;
      }
      if (action == "get")
      {
        sendLobbySnapshotToAll();
        return;
      }
    }

    if (cmd == "move")
    {
      int player = doc["data"] | -1;
      Serial.printf("player move: %d\n", player);

      StaticJsonDocument<128> out;
      out["type"] = "moveOk";
      out["player"] = player;
      broadcastJson(out);
      return;
    }
    if (cmd == "startGame")
    {
      gameStarted = true;
      StaticJsonDocument<128> out;
      out["type"] = "start";
      out["ok"] = true;
      broadcastJson(out);
      Serial.println("Spiel startet (JSON)!");
      return;
    }
    if (cmd == "settime")
    {
      unsigned long t = doc["t"] | 0UL;
      if (t > 1000000000UL)
      { // plausibler Unix-Timestamp (nach 2001)
        struct timeval tv;
        tv.tv_sec = (time_t)t;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr);
        Serial.printf("[TIME] Systemzeit gesetzt: %lu\n", t);
        batterySendRequested = true; // jetzt erst Snapshot – Timestamp ist korrekt
      }
      return;
    }

    if (cmd == "auswahl")
    {
      auswahl = doc["data"];
      Serial.printf("figur ausgewählt: %d\n", auswahl);
      return;
    }

    if (cmd == "selected")
    {
      int chos = doc["data"];
      chos = chos - 1;
      alle.changeCurrentFigur(chos);
      return;
    }

    // unbekannter JSON command
    StaticJsonDocument<128> out;
    out["type"] = "error";
    out["msg"] = "unknown json command";
    broadcastJson(out);
    return;
  }

  // ======================================================
  // 2) KEIN JSON → normale Text-Kommandos
  // ======================================================

  if (msg == "roll")
  {
    rolled = true;
    return;
  }
  else if (msg == "startGame")
  {
    gameStarted = true;

    StaticJsonDocument<128> doc;
    doc["type"] = "start";
    doc["ok"] = true;
    broadcastJson(doc);

    float v = batteryReadVoltage();
    int pct = batteryPercent(v);
    // lcdDiceShowBattery(v, pct);
    return;
  }
  else if (msg == "sleep")
  {
    changetosleep = true;
    Serial.println("Sleep command received, will enter light sleep mode.");
    return;
  }
  else if (msg == "switch")
  {
    JsonDocument doc;
    doc["type"] = "action";
    doc["action"] = "switch";
    broadcastJson(doc);
    return;
  }
  else if (msg == "done")
  {
    JsonDocument doc;
    doc["type"] = "action";
    doc["action"] = "done";
    broadcastJson(doc);

    float v = batteryReadVoltage();
    int pct = batteryPercent(v);
    // lcdDiceShowBattery(v, pct);
    return;
  }
  else if (msg == "battery?")
  {
    batterySendRequested = true; // trigger measurement in loop
    return;
  }

  // ------------------------------------------------------
  // Fallback → echo
  // ------------------------------------------------------
  StaticJsonDocument<128> doc;
  doc["type"] = "echo";
  doc["msg"] = msg;
  broadcastJson(doc);
}

// ------------------- WebSocket Event Handler -------------------
void onEvent(AsyncWebSocket *serverPtr, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("WS: Client #%u connected\n", client->id());
    StaticJsonDocument<128> doc;
    doc["type"] = "welcome";
    doc["msg"] = "Willkommen beim Ludo-Server";
    String out;
    serializeJson(doc, out);
    client->text(out);
    sendLobbySnapshotToClient(client);

    // Wenn Spiel läuft: aktuellen Spieler an den neuen Client schicken
    if (spielRunning)
    {
      Datenschicken("spieler", alle.currentPlayer);
    }
    // batterySendRequested wird erst im settime-Handler gesetzt,
    // damit der Timestamp korrekt ist wenn der Snapshot geschrieben wird.
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.println("WS: Client disconnected");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len)
    {
      handleWsMessage(arg, data, len);
    }
  }
}

// ------------------- WiFi + Webserver Setup -------------------
void setupWiFi()
{

  // 1) Access Point starten
  WiFi.softAP(ssid, password);
  configTime(3600, 3600, "pool.ntp.org"); // UTC+1 Wien, Sommerzeit
  Serial.println("Access Point gestartet!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.softAPIP());

  // 3) LittleFS mounten usw.
  if (!LittleFS.begin(true))
  {
    Serial.println("Fehler beim Mounten von LittleFS");
    return;
  }

  // Debug Files
  Serial.printf("index.html: %d\n", LittleFS.exists("/index.html"));
  Serial.printf("register.html: %d\n", LittleFS.exists("/register.html"));
  Serial.printf("dashboard.html: %d\n", LittleFS.exists("/dashboard.html"));
  Serial.printf("battery.html: %d\n", LittleFS.exists("/battery.html"));
  Serial.printf("profile.html: %d\n", LittleFS.exists("/profile.html"));
  Serial.printf("stats.html: %d\n", LittleFS.exists("/stats.html"));
  Serial.printf("style.css: %d\n", LittleFS.exists("/style.css"));
  Serial.printf("js/utils.js: %d\n", LittleFS.exists("/js/utils.js"));

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Pages
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
  server.on("/register.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/register.html", "text/html"); });
  server.on("/dashboard.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/dashboard.html", "text/html"); });
  server.on("/profile.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/profile.html", "text/html"); });
  server.on("/stats.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/stats.html", "text/html"); });
  server.on("/battery.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/battery.html", "text/html"); });
  server.on("/pregame.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/pregame.html", "text/html"); });
  server.on("/game.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/game.html", "text/html"); });
  server.on("/spiel.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/spiel.html", "text/html"); });
  // Assets
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/style.css", "text/css"); });
  server.serveStatic("/js", LittleFS, "/js").setCacheControl("no-store");

  server.on("/api/battery", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", sdcard_getBatteryJson()); });

  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(404, "text/plain", "Not found"); });

  server.begin();
  randomSeed(esp_random());
  Serial.println("Webserver + WebSocket gestartet!");
}

void unsetupWiFi()
{
  ws.enable(false);            // keine neuen WS-Verbindungen annehmen
  ws.closeAll();               // alle offenen Clients sauber trennen
  ws.cleanupClients();         // getrennte Clients freigeben
  server.end();                // HTTP-Server stoppen
  WiFi.softAPdisconnect(true); // AP abschalten
  WiFi.mode(WIFI_OFF);         // WiFi komplett aus
  Serial.println("WiFi und Webserver gestoppt.");
}

void setupPeriphery()
{
  // Akku/LED initialisieren
  batteryInit();
  lcdDiceInit();
}

void setupAll()
{
  // SD-Karte mounten (spiSD.begin() wird intern in microsdcard_init() aufgerufen)
  microsdcard_init();

  setupPeriphery();
  setupHardware();

  // .gz-Suche von ESPAsyncWebServer unterdruecken (nur Spam, kein echter Fehler)
  esp_log_level_set("vfs_api", ESP_LOG_NONE);

  if (checkWifiLever())
  {
    setupWiFi();
  }
}

void OnlineMode(int mode)
{
  if (mode)
  {
    setupWiFi();
  }
  else
  {
    unsetupWiFi();
  }
}

// ------------------- Zyklische Akku-Updates -------------------
void wifi_ap_loop_batteryTick()
{
  const unsigned long periodMs = 15000;
  static unsigned long lastCleanup = 0;

  bool doSend = false;

  if (millis() - lastCleanup > 200)
  {
    ws.cleanupClients();
    lastCleanup = millis();
  }

  static unsigned long last = 0;

  if (millis() - lastBatMs >= periodMs)
  {
    lastBatMs = millis();
    doSend = true;
  }
  if (batterySendRequested)
  {
    batterySendRequested = false;
    doSend = true;
  }
  if (doSend)
  {
    sendBatterySnapshot();
  }
  if (changetosleep)
  {
    changetosleep = false;
    enterLightSleep();
  }
}

// ------------------- Datenschicken (mit Farbe & Array) -------------------
void Datenschicken(String type, int farbe, int intdata[], size_t len)
{
  StaticJsonDocument<192> doc;
  doc["type"] = type;
  doc["farbe"] = farbe;

  // JSON-Array erstellen
  JsonArray arr = doc.createNestedArray("intdata");
  for (size_t i = 0; i < len; i++)
  {
    arr.add(intdata[i]);
  }

  broadcastJson(doc);
  // noch keine Implementierung
}

// ------------------- Datenschicken (ohne Farbe, nur ein Wert) -------------------
void Datenschicken(String type, int intdata)
{
  StaticJsonDocument<192> doc;
  doc["type"] = type;
  doc["value"] = intdata;
  doc["intdata"] = intdata;

  broadcastJson(doc);
  // noch keine Implementierung
}

// debug serial

String serialBuffer = "";

void handleJsonMessage(const JsonDocument &doc)
{
  if (!doc.containsKey("cmd"))
  {
    Serial.println("JSON ohne cmd-Feld erhalten!");
    return;
  }

  String cmd = doc["cmd"].as<String>();
  Serial.print("JSON command: ");
  Serial.println(cmd);

  // -------------------------------------------------------
  // switch
  // -------------------------------------------------------
  if (cmd == "switch")
  {
    StaticJsonDocument<128> out;
    out["type"] = "action";
    out["action"] = "switch";
    broadcastJson(out);
    return;
  }

  // -------------------------------------------------------
  // done
  // -------------------------------------------------------
  if (cmd == "done")
  {
    StaticJsonDocument<128> out;
    out["type"] = "action";
    out["action"] = "done";
    broadcastJson(out);
    return;
  }

  // -------------------------------------------------------
  // auswahl → JSON-Version: {"cmd":"a","v":3}
  // -------------------------------------------------------
  if (cmd == "a")
  {
    if (!doc.containsKey("v"))
    {
      Serial.println("JSON auswahl ohne value!");
      return;
    }

    auswahl = doc["v"].as<int>();
    Serial.printf("auswahl: %d\n", auswahl);
    return;
  }

  // -------------------------------------------------------
  // move
  // -------------------------------------------------------
  if (cmd == "move")
  {
    int auswahl = doc["player"] | -1;
    Serial.printf("Spieler %d wird bewegt!\n", auswahl);

    StaticJsonDocument<128> out;
    out["type"] = "moveOk";
    out["player"] = auswahl;
    broadcastJson(out);
    return;
  }

  // -------------------------------------------------------
  // unbekannt → echo JSON
  // -------------------------------------------------------
  StaticJsonDocument<128> out;
  out["type"] = "echo";
  out["cmd"] = cmd;
  broadcastJson(out);
}

void handleSerialMessage(const String &msg)
{
  // Zuerst prüfen: ist es JSON?
  if (msg.startsWith("{") && msg.endsWith("}"))
  {

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, msg);

    if (err)
    {
      Serial.print("JSON Parsing Error: ");
      Serial.println(err.c_str());
      return;
    }

    handleJsonMessage(doc);
    return;
  }

  //  Normale Textbefehle
  Serial.print("Text command received: ");
  Serial.println(msg);

  if (msg == "s")
  {
    gameStarted = true;

    StaticJsonDocument<128> out;
    out["type"] = "start";
    out["ok"] = true;
    broadcastJson(out);
    Serial.println("Spiel startet!");
    return;
  }
  else if (msg == "r")
  {
    rolled = true;
    Serial.println("Spiel gerollt!");
  }
  else if (msg == "battery")
  {
    Serial.println("Batteriestand wird abgefragt…");
    batterySendRequested = true; // trigger measurement in loop
  }
  else if (msg == "sleep")
  {
    enterLightSleep();
  }
  else
  {
    Serial.println("Unbekannter Text-Befehl!");
  }
}

// ----------------- Serial Reader -----------------

void serialInputLoop()
{
  while (Serial.available() > 0)
  {

    char c = Serial.read();

    // Nachricht abgeschlossen (ENTER gedrückt)
    if (c == '\n' || c == '\r')
    {
      if (serialBuffer.length() > 0)
      {
        handleSerialMessage(serialBuffer);
        serialBuffer = "";
      }
    }
    else
    {
      serialBuffer += c;
    }
  }
}
