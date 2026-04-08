#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "esp_sleep.h"
#include "driver/gpio.h"

#include "hardware.h"
#include "wifi_ap.h"
#include "lcd_dice.h"

#define LED_PIN 7
#define NUM_LEDS 60
#define NUM_LEDS_STRIP 17

#define WIFILEVER_PIN 4

// Spielerfarben
#define ROT 0
#define BLAU 1
#define GRUEN 2
#define GELB 3

#define DEC_PIN1 16
#define DEC_PIN2 17

#define BTN_SELECT 36 // IO36 – Auswählen

#define BTN_NEXT 37 // IO37 – Wechseln

#define BTN_CANCEL 38 // IO38 – Abbrechen

#define DEBOUNCE_MS 50

bool WifiSelected;

static unsigned long lastPressTime[3] = {0, 0, 0};

bool lastState[3] = {LOW, LOW, LOW};

const int btnPins[3] = {BTN_SELECT, BTN_NEXT, BTN_CANCEL};

static Adafruit_NeoPixel strip(NUM_LEDS_STRIP, LED_PIN, NEO_GRB + NEO_KHZ800);

// Farben für NeoPixel
uint32_t colors[] = {
    strip.Color(0, 0, 0),      // frei
    strip.Color(255, 0, 0),    // Rot
    strip.Color(0, 0, 255),    // Blau
    strip.Color(0, 255, 0),    // Grün
    strip.Color(255, 255, 0),  // Gelb
    strip.Color(255, 255, 255) // weiß
};

const int quadrantOffset[4] = {
    0,  // rot:   5–14
    15, // blau:  20–29
    30, // grün:  35–44
    45  // gelb:  50–59
};

int LEDFelder[60] = {0}; // 0-rotstartfeld 15-blau 30-grün 45-gelb //nummer ist anzahl der figuren im start //z.b 2=> 2 figuren im start
                         // 1-4-rotzielfeld 16-19-blau 31-34-grün 46-49-gelb // für jedes feld wird der spieler eingetragen //z.B. 1110 => figuren sind in feldern 1 bis 3
                         // 5-14-rotquadrant 20-29-blau 35-44-grün 50-59-gelb // farbe der jeweiligen spieler wird eingetragen //z.B. 0001000300 farbe 1 und 3 sind in diesem quadrant

void IRAM_ATTR WifiSelectLever()
{
    if (!spielRunning)
    {
        OnlineMode(checkWifiLever());
    }
}

void ledsteuerung()
{
    int ledfeld[72] = {0};
    int k = 0;
    printf("\n");

    for (int i = 0; i < NUM_LEDS; i++)
    {
        int f = LEDFelder[i];

        if (i % 15 == 0)
        {
            int quadcolour = (i / 15) + 1;
            for (int j = 0; j < 4; j++)
            {
                if (j < f)
                {
                    if (k + i < NUM_LEDS_STRIP)
                    {
                        strip.setPixelColor(i + k, colors[quadcolour]);
                    }
                    ledfeld[i + k] = colors[quadcolour];
                    printf("%d ", k + i);
                }
                else
                {
                    if (k + i < NUM_LEDS_STRIP)
                    {
                        strip.setPixelColor(i + k, colors[0]);
                    }
                    ledfeld[i + k] = colors[0];
                    printf("%d ", k + i);
                }
                if (j < 3)
                {
                    k++;
                }
            }
        }
        else
        {
            if (k + i < NUM_LEDS_STRIP)
            {
                strip.setPixelColor(i + k, colors[f]);
            }
            ledfeld[i + k] = colors[f];
            printf("%d ", k + i);
        }
    }
    printf("\nLEDFELD auf brett\n");
    for (int j = 0; j < NUM_LEDS_STRIP; j++)
    {
        printf("%d ", ledfeld[j]);
    }
    printf("\n");
    strip.show();
}

void sendLEDdata(int ring[], int start[], int ziel[][4])
{
    int j = 0;
    for (int i = 0; i < 60; i++)
    {
        if (i % 15 == 0)
        {
            LEDFelder[i] = start[i / 15];
        }
        else if (i % 15 > 0 && i % 15 < 5)
        {
            LEDFelder[i] = ziel[i / 15][(i % 15) - 1];
        }
        else
        {
            LEDFelder[i] = ring[j];
            j++;
        }
    }
    Serial.printf("LEDFELDER: ");
    for (int i = 0; i < 60; i++)
        Serial.printf("%d ", LEDFelder[i]);
    Serial.printf("\n");

    ledsteuerung();

    return;
}

// 0-in Reserve, 1-in feld, 2-im Ziel
int figPosToLEDIndex(int typ, Figur figur)
{
    if (typ == 0)
    {
        int farbquad = quadrantOffset[figur.farbe];
        int anz = LEDFelder[farbquad];
        if (anz == 0)
        {
            return -1;
        }
        return anz + farbquad + 3 * (figur.farbe) - 1;
    }
    else if (typ == 1)
    {
        int farbquad = (figur.posRing / 10) + 1;
        return figur.posRing + 8 * farbquad;
    }
    else if (typ == 2)
    {
        return figur.zielfeldIndex + 8 * figur.farbe + startPos[figur.farbe] + 4;
    }
    return -1;
}

void highlightFigur(int figurIndex)
{

    // Aktuelle Spielfelddaten laden

    alle.printAll(); // LEDFelder[] aktualisieren

    // Ausgewaehlte Figur blinken lassen (weiss ueberlagern)

    Spieler &sp = alle.spieler[alle.currentPlayer];

    Figur &f = sp.figuren[figurIndex];

    int ledIndex = -1;

    if (f.posRing == -1 && !f.imZiel)
    {

        // Figur in der Reserve: LED-Position berechnen

        ledIndex = figPosToLEDIndex(0, f);
        if (ledIndex == -1)
        {
            printf("figPosLedIndexFehler");
            return;
        }

        strip.setPixelColor(ledIndex, colors[5]); // Weiss = markiert

        strip.show();
    }
    else if (f.posRing >= 0 && !f.imZiel)
    {
        ledIndex = figPosToLEDIndex(1, f);
        if (ledIndex == -1)
        {
            printf("figPosLedIndexFehler");
            return;
        }

        strip.setPixelColor(ledIndex, colors[5]); // Weiss = markiert

        strip.show();
    }
    else if (f.posRing == -1 && f.imZiel)
    {
        ledIndex = figPosToLEDIndex(2, f);
        if (ledIndex == -1)
        {
            printf("figPosLedIndexFehler");
            return;
        }

        strip.setPixelColor(ledIndex, colors[5]); // Weiss = markiert

        strip.show();
    }
}

void handleButtonPress(int btn)
{

    // Button 2 (Wechseln): naechste Figur markieren

    if (btn == 1)
    {
        printf("change");

        if (turnround == 2)
        {

            // Naechste bewegbare Figur auswaehlen

            alle.changeCurrentFigur();
            printf("\nchangefigtonext\n");
        }

        return;
    }

    // Button 3 (Abbrechen): Auswahl zuruecksetzen

    else if (btn == 2)
    {
        printf("cancel");

        if (turnround == 2)
        {
            alle.changeCurrentFigur(0);
            printf("\backtofig1\n");
        }

        return;
    }

    // Button 1 (Auswaehlen / Bestaetigen)

    else if (btn == 0)
    {
        printf("auswahl");
        if(gameStarted == false)
        {
            gameStarted = true;
            return;
        }
        else if (turnround == 1)
        {

            // Phase 1: Wuerfeln ausloesen

            rolled = true;
            printf("\nbuttonrolles\n");
        }
        else if (turnround == 2)
        {

            // Phase 2: Figur bestaetigen

            auswahl = currentFigur[0] + 1; // 1-basiert
            printf("\nauswahl: %d\n", auswahl);
        }
    }
}

void buttonInputLoop()
{
    if (WifiSelected)
        return;

    if (lcdBusy)
        return;

    unsigned long now = millis();

    for (int i = 0; i < 3; i++)
    {
        bool state = digitalRead(btnPins[i]);

        // Steigende Flanke erkannt (LOW -> HIGH = Tastendruck)
        if (state == HIGH && lastState[i] == LOW)
        {
            if (now - lastPressTime[i] >= DEBOUNCE_MS)
            {
                lastPressTime[i] = now;
                handleButtonPress(i);
            }
        }

        lastState[i] = state;
    }
}

void playerButtonLock(int player)
{
    if (WifiSelected)
    {
        return;
    }
    digitalWrite(DEC_PIN1, (player >> 0) & 1); // Bit 0
    digitalWrite(DEC_PIN2, (player >> 1) & 1); // Bit 1
}

void enterLightSleep()
{
    if (gameStarted)
    {
        strip.clear();
        strip.show();
        playerButtonLock(0); // alle Spieler entsperren
    }

    lcdDiceOff();
    if (WifiSelected)
    {
        unsetupWiFi();
    }
    while (true)
    {
        Serial.println("Gehe in Light Sleep...");
        Serial.flush();

        gpio_wakeup_enable(gpio_num_t(BTN_CANCEL), GPIO_INTR_HIGH_LEVEL); // LOW = gedrückt
        gpio_wakeup_enable(gpio_num_t(BTN_NEXT), GPIO_INTR_HIGH_LEVEL);
        gpio_wakeup_enable(gpio_num_t(BTN_SELECT), GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();

        delay(200); // warten bis Pins stabil sind
        esp_light_sleep_start();

        unsigned long startTime = millis();
        bool losgelassen = false;

        while (millis() - startTime < 5000)
        {
            if (digitalRead(BTN_CANCEL) == LOW &&
                digitalRead(BTN_NEXT) == LOW &&
                digitalRead(BTN_SELECT) == LOW)
            {
                losgelassen = true; // kein Button mehr gedrückt
                Serial.println("Button los");
                break;
            }
            delay(50);
        }

        if (!losgelassen)
        {
            break; // 5s gehalten → aufwachen
        }
        Serial.println("Button losgelassen – zurück in Light Sleep.");
    }

    Serial.println("Aufgewacht!");

    lcdDiceInit(); // Display wieder einschalten

    if (gameStarted)
    {
        alle.printAll(); // LEDs aktualisieren
        playerButtonLock(alle.currentPlayer);
    }
    if (WifiSelected)
    {
        setupWiFi();
    }
}

int checkWifiLever()
{
    WifiSelected = digitalRead(WIFILEVER_PIN);
    return WifiSelected;
}

void setupHardware()
{
    pinMode(LED_PIN, OUTPUT);

    // NeoPixel Setup
    strip.begin();
    strip.setBrightness(50);
    strip.show(); // alle LEDs aus

    // Taster-Pins als Eingang mit Pull-Up konfigurieren

    pinMode(BTN_SELECT, INPUT);

    pinMode(BTN_NEXT, INPUT);

    pinMode(BTN_CANCEL, INPUT);

    pinMode(DEC_PIN1, OUTPUT);

    pinMode(DEC_PIN2, OUTPUT);

    digitalWrite(DEC_PIN1, LOW);
    digitalWrite(DEC_PIN2, LOW);

    // Kippschalter-Interrupt

    attachInterrupt(WIFILEVER_PIN, WifiSelectLever, CHANGE);

    return;
}
