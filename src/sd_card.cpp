#include "sd_card.h"
#include <SPI.h>
#include <SD.h>

// --- CYD SD Card Pins (default VSPI) ---
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS   5

// --- CYD Touch Pins (remapped VSPI) ---
#define TOUCH_SCK  25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32
#define TOUCH_CS   33

// Shared VSPI bus — declared extern, created in main.cpp
extern SPIClass touchSPI;

static bool s_sdReady = false;

bool sdCardBegin() {
    // Configure VSPI for SD card pins
    touchSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS, touchSPI, 80000000)) {
        Serial.println("[SD] Card mount failed");
        s_sdReady = false;
        // Restore touch pins even on failure
        touchSPI.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] No card attached");
        s_sdReady = false;
        touchSPI.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
        return false;
    }

    Serial.printf("[SD] Card type: %d, Size: %lluMB\n", cardType,
                  SD.cardSize() / (1024 * 1024));

    // Create save directory if it doesn't exist
    if (!SD.exists(SAVE_DIR)) {
        SD.mkdir(SAVE_DIR);
    }

    s_sdReady = true;

    // Switch back to touch pins after init
    touchSPI.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);

    Serial.println("[SD] Init OK");
    return true;
}

void sdSwitchToSD() {
    touchSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
}

void sdSwitchToTouch() {
    touchSPI.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
}

bool sdCardReady() {
    return s_sdReady;
}
