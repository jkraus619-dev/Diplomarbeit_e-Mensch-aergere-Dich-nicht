#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include "spiellogik.h"

// Flag, ob das Spiel gestartet werden soll
extern bool WifiSelected; // wenn ein spielgestartet wird


void highlightFigur(int figurIndex);

void sendLEDdata(int ring[], int start[], int ziel[][FARBFELDSIZE]);

void setupHardware();

int checkWifiLever();

void playerButtonLock(int player);

void buttonInputLoop();

void enterLightSleep();

void IRAM_ATTR WifiSelectLever();

#endif