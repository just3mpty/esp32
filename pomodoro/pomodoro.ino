/*
 * Pomodoro tactile — ESP32-S3 + écran ILI9488 + dalle tactile XPT2046
 *
 * Fichiers du croquis :
 *   pomodoro.ino  déclarations globales, setup()/loop()
 *   config.h      brochage, couleurs, géométrie, constantes de réglage
 *   types.h       enums d'état et petites structures partagées
 *   display.ino   tout le dessin (écrans principal/réglages, animations)
 *   timer.ino     machine à états du minuteur (phases, démarrage/pause)
 *   touch.ino     lecture tactile calibrée, calibrage, anti-rebond, routage des appuis
 *   storage.ino   sauvegarde des réglages et du calibrage (NVS)
 *   animations.h  images générées (ne pas modifier à la main)
 */

#include <SPI.h>
#include <Preferences.h>
#include <XPT2046_Touchscreen.h>
#include <Arduino_GFX_Library.h>
#include <AnimatedGIF.h>
#include "types.h"
#include "config.h"
#include "animations.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, GFX_NOT_DEFINED, HSPI);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, TFT_RST, 1 /* paysage */, false);
XPT2046_Touchscreen ts(TOUCH_CS);
Preferences prefs;
AnimatedGIF fondGif, zzzGif;  // fond fixe plein écran + « z » animés du chat
// Bande du ciel de la scène de travail : dessinée en mémoire puis envoyée d'un bloc
Arduino_Canvas *sky = new Arduino_Canvas(480, SKY_H, gfx, 0, 0);

const Button BTN_START    = {330,  60, 140, 70};
const Button BTN_STOP     = {330, 145, 140, 50};
const Button BTN_SKIP     = {330, 205, 140, 50};
const Button BTN_SETTINGS = {330, 265, 140, 45};

// Écran des réglages : une ligne par valeur, avec [-] et [+]
const char *SETTING_NAMES[SETTING_COUNT] = {"Travail", "Pause", "Grande pause", "Sessions"};
const char *SETTING_UNITS[SETTING_COUNT] = {"min", "min", "min", ""};
const uint8_t SETTING_MIN[SETTING_COUNT] = {1, 1, 1, 1};
const uint8_t SETTING_MAX[SETTING_COUNT] = {90, 30, 60, 8};
int rowY(int i) { return 55 + i * 53; }
Button minusBtn(int i) { return {250, (int16_t)rowY(i), 55, 45}; }
Button plusBtn(int i)  { return {415, (int16_t)rowY(i), 55, 45}; }
const Button BTN_CALIBRATE = {10, 270, 200, 45};
const Button BTN_OK        = {290, 270, 180, 45};

// Réglages : travail, pause, grande pause (minutes), sessions par cycle
uint8_t settings[SETTING_COUNT] = {25, 5, 15, 4};
Calibration cal;

Phase phase = WORK;
State state = READY;
Screen screen = MAIN;
uint32_t phaseMs;            // durée totale de la phase
uint32_t elapsedMs = 0;      // temps écoulé, hors pauses
uint32_t runStartMs = 0;     // millis() au dernier démarrage / reprise
uint32_t elapsedAtStart = 0; // elapsedMs au dernier démarrage / reprise
int doneInCycle = 0;         // sessions de travail terminées dans le cycle en cours
int doneTotal = 0;           // sessions de travail terminées depuis l'allumage

float arcDrawnDeg = 0;       // portion de l'anneau déjà grisée
uint32_t nextGifMs = 0;      // millis() de la prochaine image de l'animation
uint32_t lastTouchMs = 0;    // millis() du dernier appui sur l'écran
int shownSeconds = -1;       // dernière valeur affichée du compte à rebours

uint16_t phaseColor() {
  switch (phase) {
    case WORK: return COL_WORK;
    case SHORT_BREAK: return COL_SHORT;
    default: return COL_LONG;
  }
}

const char *phaseName() {
  switch (phase) {
    case WORK: return "TRAVAIL";
    case SHORT_BREAK: return "PAUSE";
    default: return "GRANDE PAUSE";
  }
}

uint32_t phaseDurationMs(Phase p) {
  int m = p == WORK ? settings[0] : p == SHORT_BREAK ? settings[1] : settings[2];
  return (uint32_t)m * SECONDS_PER_MINUTE * 1000;
}

bool isBreak() { return phase != WORK; }

void beep(int count) {
  if (BUZZER_PIN < 0) return;
  for (int i = 0; i < count; i++) {
    tone(BUZZER_PIN, 2000, 150);
    delay(300);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  if (!gfx->begin(40000000)) Serial.println("Echec init ecran");
  if (!sky->begin(GFX_SKIP_OUTPUT_BEGIN)) Serial.println("Echec allocation du ciel");
  fondGif.begin(GIF_PALETTE_RGB565_LE);
  zzzGif.begin(GIF_PALETTE_RGB565_LE);
  if (!zzzGif.open((uint8_t *)CHAT_ZZZ, sizeof(CHAT_ZZZ), gifDraw)) Serial.println("Echec ouverture GIF");

  // Démarrer le bus partagé sur ses broches AVANT ts.begin(), sinon la
  // bibliothèque l'ouvrirait sur les broches par défaut (celles de l'écran)
  SPI.begin(BUS_SCK, BUS_MISO, BUS_MOSI);
  ts.begin(SPI);

  loadPrefs();
  if (cal.ax == 0 || digitalRead(BTN_PIN) == LOW) calibrate();
  startPhase(WORK);
}

void loop() {
  pollTouch();
  pollBootButton();

  if (state == RUNNING) {
    elapsedMs = elapsedAtStart + (millis() - runStartMs);
    if (screen == MAIN) {
      drawTime();
      drawArcProgress();
      if (millis() - lastTouchMs > 8000) enterAnim();
    } else if (screen == ANIM) {
      drawTime();
      animTick();
    }
    if (elapsedMs >= phaseMs) {
      Serial.printf("Fin de la phase : %s\n", phaseName());
      flashEnd();
      nextPhase();
    }
  }
  delay(10);
}
