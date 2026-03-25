#include "lcd_dice.h"
#include "battery.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

volatile bool lcdBusy = false;

// Parameter vom Display und Würfel (Farben, Größen, Verzögerungen)
static constexpr int DICE_SIZE = 160;
static constexpr int DICE_CORNER_RADIUS = 24;
static constexpr int DICE_DOT_RADIUS = 14;
static constexpr uint16_t COLOR_DICE = GC9A01A_WHITE;
static constexpr uint16_t COLOR_DICE_BORDER = GC9A01A_WHITE;
static constexpr uint16_t COLOR_DICE_DOT = GC9A01A_BLACK;

static constexpr int DICE_STEPS = 20;
static constexpr int FAST_DELAY_MS = 60;
static constexpr int MEDIUM_DELAY_MS = 200;
static constexpr int SLOW_DELAY_1_MS = 400;
static constexpr int SLOW_DELAY_2_MS = 650;
static constexpr int SLOW_DELAY_3_MS = 950;

static Adafruit_GC9A01A tft(LCD_TFT_CS, LCD_TFT_DC, LCD_TFT_RST);
static bool lcdReady = false;
static int gridCenterX = 0;
static int gridCenterY = 0;
static int gridStep = 0;
static uint16_t currentBgColor = GC9A01A_BLACK;
static const uint16_t BG_COLORS[] = {
    GC9A01A_RED,
    GC9A01A_BLUE,
    GC9A01A_GREEN,
    GC9A01A_YELLOW,
    GC9A01A_BLACK};
static size_t bgIndex = 0;

// gibt Text horizontal zentriert auf dem Display aus
static void printCentered(const char *text, int y)
{
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((tft.width() - (int16_t)w) / 2, y);
  tft.print(text);
}

// zeichnet den Würfel-Hintergrund
static void drawDiceBackground()
{
  int originX = (tft.width() - DICE_SIZE) / 2;
  int originY = (tft.height() - DICE_SIZE) / 2;
  gridCenterX = originX + DICE_SIZE / 2; // Zentrieren
  gridCenterY = originY + DICE_SIZE / 2; // Zentrieren
  gridStep = DICE_SIZE / 4;              // Abstand der Würfelaugen

  tft.fillRoundRect(originX, originY, DICE_SIZE, DICE_SIZE, DICE_CORNER_RADIUS, COLOR_DICE);
  tft.drawRoundRect(originX, originY, DICE_SIZE, DICE_SIZE, DICE_CORNER_RADIUS, COLOR_DICE_BORDER);
}

// zeichent einen Würfelpunkt an der gegebenen Position
static void drawDot(int dx, int dy)
{
  int x = gridCenterX + dx * gridStep;
  int y = gridCenterY + dy * gridStep;
  tft.fillCircle(x, y, DICE_DOT_RADIUS, COLOR_DICE_DOT);
}

// plaziert die Würfelaugen entsprechend dem Wert
static void drawDiceFace(int face)
{
  drawDiceBackground();

  switch (face)
  {
  case 1:
    drawDot(0, 0);
    break;
  case 2:
    drawDot(-1, -1);
    drawDot(1, 1);
    break;
  case 3:
    drawDot(-1, -1);
    drawDot(0, 0);
    drawDot(1, 1);
    break;
  case 4:
    drawDot(-1, -1);
    drawDot(1, -1);
    drawDot(-1, 1);
    drawDot(1, 1);
    break;
  case 5:
    drawDot(-1, -1);
    drawDot(1, -1);
    drawDot(0, 0);
    drawDot(-1, 1);
    drawDot(1, 1);
    break;
  case 6:
  default:
    drawDot(-1, -1);
    drawDot(1, -1);
    drawDot(-1, 0);
    drawDot(1, 0);
    drawDot(-1, 1);
    drawDot(1, 1);
    break;
  }
}

// startet und initialisiert das Display
void lcdDiceInit()
{
  SPI.begin(LCD_TFT_SCLK, -1, LCD_TFT_MOSI); //-1, weil MISO nicht benutzt wird
  tft.begin(40000000);                         // 40 MHz SPI
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);

  float v = batteryReadVoltage();
  int pct = batteryPercent(v);


  lcdDiceShowBattery(v, pct);

  lcdReady = true;
}

// spielt die Würfel-Animation ab und endet bei finalValue (1..6)
void lcdDiceRoll(int finalValue, int currentplayer)
{
  if (lcdBusy)
  {
    Serial.println("lcdDiceRoll: BLOCKED - lcdBusy ist true!");
    return;
  }
  lcdBusy = true;

  if (!lcdReady)
  {
    lcdDiceInit();
  }
  if (finalValue < 1 || finalValue > 6)
  {
    finalValue = (abs(finalValue) % 6) + 1;
  }

  currentBgColor = BG_COLORS[currentplayer];
  tft.fillScreen(currentBgColor);

  for (int i = 0; i < DICE_STEPS; i++)
  {
    int value = random(1, 7);
    drawDiceFace(value);
    delay(1); // kurz warten, damit das Display Zeit zum Zeichnen hat

    if (i < DICE_STEPS - 3)
    {
      int stepDelay = map(i, 0, DICE_STEPS - 4, FAST_DELAY_MS, MEDIUM_DELAY_MS);
      delay(stepDelay);
    }
    else if (i == DICE_STEPS - 3)
    {
      delay(SLOW_DELAY_1_MS);
    }
    else if (i == DICE_STEPS - 2)
    {
      delay(SLOW_DELAY_2_MS);
    }
    else
    {
      delay(SLOW_DELAY_3_MS);
    }
  }

  drawDiceFace(finalValue);
  lcdBusy = false;
}

// Meldung ob das Display gerade beschäftigt ist
bool lcdDiceIsBusy()
{
  return lcdBusy;
}

// zeigt Batteriestatus an
void lcdDiceShowBattery(float voltage, int percent)
{
  if (lcdBusy)
    return;
  lcdBusy = true;

  if (!lcdReady)
  {
    lcdDiceInit();
  }

  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);

  char line[32];
  int midY = tft.height() / 2;
  printCentered("Batterie:", midY - 20);

  snprintf(line, sizeof(line), "%.2f V", voltage);
  printCentered(line, midY + 0);

  snprintf(line, sizeof(line), "%d %%", percent);
  printCentered(line, midY + 22);

  lcdBusy = false;
}

// zeigt den aktuellen Spieler an
void lcdDiceShowCurrentPlayer(int player)
{
  if (lcdBusy)
    return;
  lcdBusy = true;

  if (!lcdReady)
  {
    lcdDiceInit();
  }

  static const char *playerColors[] = {"Rot", "Blau", "Gruen", "Gelb"};
  int idx = (player >= 0 && player <= 3) ? player : 0;

  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);

  int midY = tft.height() / 2;
  printCentered("Spieler", midY - 24);
  printCentered(playerColors[idx], midY - 4);
  printCentered("ist dran!", midY + 16);

  lcdBusy = false;
}