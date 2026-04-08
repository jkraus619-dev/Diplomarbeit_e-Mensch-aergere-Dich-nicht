#include <Arduino.h>
#include "wifi_ap.h"
#include "spiellogik.h"
#include "lcd_dice.h"
#include "hardware.h"

// Vorwärtsdeklaration (Tick aus wifi_ap.cpp)
void wifi_ap_loop_batteryTick();

void setup()
{
  Serial.begin(115200);
  delay(100);
  setupAll();
}

void loop()
{
  // Websocket + WiFi Tick
  wifi_ap_loop_batteryTick();
  serialInputLoop();
  buttonInputLoop();

  // Spiel starten, falls Flag gesetzt
  if (gameStarted && !spielRunning)
  {
    spielRunning = true;
    gameStarted = 0;
    spielinit();
  }
  if (turnround == 1 && rolled)
  {
    if (lcdBusy)
    {
      return;
    }
    alle.wuerfeln();
    rolled = false;
    auswahl = false;
  }
  else if (turnround == 2 && auswahl)
  {
    if (lcdBusy)
    {
      return;
    }
    alle.continueTurn(auswahl);
    auswahl = false;
    rolled = false;
  }
}
