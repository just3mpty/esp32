#pragma once

// Écran : bus SPI dédié (HSPI)
#define TFT_SCK   12
#define TFT_MOSI  11
#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST   14
#define TFT_BL    21
// Tactile : bus SPI partagé (FSPI)
#define BUS_SCK    7
#define BUS_MOSI   6
#define BUS_MISO   5
#define TOUCH_CS  17

#define BTN_PIN    0   // bouton BOOT de la DevKit
#define BUZZER_PIN -1  // GPIO d'un buzzer piézo, ou -1 si aucun

// Mettre 1 pour tester : chaque « minute » ne dure alors qu'une seconde
#define SECONDS_PER_MINUTE 60

// Dalle résistive : un appui n'est validé qu'après un contact stable d'au
// moins TOUCH_CONFIRM_MS sans dériver de plus de TOUCH_JITTER_MAX, ce qui
// filtre les rebonds de contact (surtout au relâchement) et le bruit
// électrique. Aucun nouvel appui n'est accepté avant TOUCH_COOLDOWN_MS.
#define TOUCH_CONFIRM_MS   25
#define TOUCH_COOLDOWN_MS 300
#define TOUCH_JITTER_MAX   30

// Dark mode, base : noir #0f0f0f, blanc #dcd7d0, bleu #4c63d2
#define COL_BG      RGB565(0x0f, 0x0f, 0x0f)  // #0f0f0f (noir)
#define COL_TEXT    RGB565(0xdc, 0xd7, 0xd0)  // #dcd7d0 (blanc)
#define COL_DIM     RGB565(0x6b, 0x69, 0x66)  // blanc atténué, texte secondaire
#define COL_TRACK   RGB565(0x30, 0x2f, 0x2e)  // gris foncé, piste de l'anneau
#define COL_BTN     RGB565(0x3c, 0x3b, 0x39)  // gris foncé, boutons neutres
#define COL_WORK    RGB565(0x4c, 0x63, 0xd2)  // #4c63d2 (bleu)
#define COL_SHORT   RGB565(0x8b, 0x9a, 0xe2)  // bleu éclairci (pause)
#define COL_LONG    RGB565(0x34, 0x41, 0x84)  // bleu assombri (grande pause)
#define COL_CLOUD   RGB565(0xdc, 0xd7, 0xd0)
#define COL_SUN     RGB565(0xb7, 0xc1, 0xed)

// Géométrie (écran en paysage, 480 x 320)
#define CX 165
#define CY 165
#define R_OUT 145
#define R_IN  127
#define SKY_H 130

#define SETTING_COUNT 4
