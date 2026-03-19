#pragma once
#include "game_types.h"

// Initialize save manager (call after SD card init)
void saveManagerBegin();

// Load game from SD card. Returns true if a valid save was found.
bool saveManagerLoad();

// Save game to SD card (immediate)
bool saveManagerSave();

// Mark save as dirty (will be written on next tick)
void saveManagerMarkDirty();

// Call periodically — saves if dirty and enough time has passed
void saveManagerTick();

// Force an immediate save
void saveManagerForce();

// Reset for a new pet (clears save, resets to defaults)
void saveManagerNewPet();

// Delete all save data (factory reset)
void saveManagerDeleteAll();

// Get/set decay mode
DecayMode saveManagerGetDecayMode();
void saveManagerSetDecayMode(DecayMode mode);

// Get/set sound enabled
bool saveManagerGetSoundEnabled();
void saveManagerSetSoundEnabled(bool enabled);
