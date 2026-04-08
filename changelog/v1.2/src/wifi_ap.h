#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <Arduino.h>

// Flag, ob das Spiel gestartet werden soll
extern bool gameStarted; //wenn ein spielgestartet wird
extern bool rolled; //wenn gewürfelt wurde
extern int auswahl; //Figurenauswahl des Spielers


// Setup für WiFi + Webserver + WebSocket

void setupAll();


// Setup für Battery + LCD-Display

// zyklische Akku-Updates (~30 s)
void wifi_ap_loop_batteryTick();

// ------------------- Prototypen -------------------
void Datenschicken(String type, int farbe, int intdata[], size_t len); 
void Datenschicken(String type, int intdata);

void OnlineMode(int mode);
void unsetupWiFi();
void setupWiFi();


void serialInputLoop();

#endif