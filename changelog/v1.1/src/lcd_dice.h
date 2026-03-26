#pragma once
#include <Arduino.h>

// Pins für das TFT-Display (1,28 Zoll Display rund)
#ifndef LCD_TFT_CS
#define LCD_TFT_CS 10
#endif
#ifndef LCD_TFT_DC
#define LCD_TFT_DC 11
#endif
#ifndef LCD_TFT_RST
#define LCD_TFT_RST 21
#endif
#ifndef LCD_TFT_SCLK
#define LCD_TFT_SCLK 8
#endif
#ifndef LCD_TFT_MOSI
#define LCD_TFT_MOSI 19
#endif

extern volatile bool lcdBusy;


// Initialisiererung des Displays und Anzeige "ready"
void lcdDiceInit();

// spielt die Würfel-Animation ab und endet bei finalValue (1..6)
void lcdDiceRoll(int finalValue, int player);

// true wenn das Display gerade beschäftigt ist
bool lcdDiceIsBusy();

// zeigt Batteriestatus an
void lcdDiceShowBattery(float voltage, int percent);

// zeigt den aktuellen Spieler auf dem Display an
void lcdDiceShowCurrentPlayer(int player);