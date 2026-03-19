// ============================================================
// Raising Hell — CYD Edition
// Main entry point
// Hardware: ESP32 CYD (ILI9341 + XPT2046 + DFPlayer DF1201S)
// ============================================================

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "audio.h"
#include "game_types.h"
#include "touch_input.h"
#include "sd_card.h"
#include "pet.h"
#include "inventory.h"
#include "save_manager.h"
#include "game_state.h"
#include "ui_renderer.h"
#include "mini_games.h"

// === HARDWARE PINS (unchanged) ===
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define TFT_BL 21

// === HARDWARE OBJECTS ===
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite mainSprite = TFT_eSprite(&tft);
SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

// === SPRITE DIMENSIONS ===
#define SPRITE_W 180
#define SPRITE_H 160

void setup() {
    Serial.begin(115200);

    // --- Display init (unchanged) ---
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    tft.init();
    tft.setRotation(1);
    tft.invertDisplay(true);
    tft.fillScreen(TFT_BLACK);

    // --- SD Card init (MUST be before touch init — shares VSPI) ---
    // sdCardBegin() temporarily configures VSPI for SD pins,
    // then restores touch pins before returning
    sdCardBegin();

    // --- Touchscreen init (unchanged) ---
    touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touch.begin(touchSPI);
    touch.setRotation(1);

    // --- Audio init (unchanged) ---
    audio_begin();

    // --- Create sprite for pet rendering ---
    mainSprite.createSprite(SPRITE_W, SPRITE_H);
    mainSprite.setTextDatum(MC_DATUM);

    // --- Initialize game subsystems ---
    touchInputBegin();
    saveManagerBegin();
    gameStateBegin();

    Serial.println("[Main] Raising Hell CYD ready");
}

void loop() {
    // 1. Poll audio (non-blocking)
    audio_loop();

    // 2. Read touch input
    InputState input = touchInputPoll(touch);

    // 3. Run game state machine (update + draw)
    gameStateTick(input);

    // 4. Frame pacing (~30fps target)
    delay(16);
}
