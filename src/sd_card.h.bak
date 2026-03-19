#pragma once
#include <Arduino.h>
#include "game_types.h"

// Initialize SD card on VSPI (pins 18/19/23, CS=5)
// Must be called BEFORE touch SPI init (they share VSPI hardware)
bool sdCardBegin();

// Switch VSPI bus to SD card pins (call before any SD operation)
void sdSwitchToSD();

// Switch VSPI bus back to touch pins (call after SD operations)
void sdSwitchToTouch();

// Check if SD card is available
bool sdCardReady();
