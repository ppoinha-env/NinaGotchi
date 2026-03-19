#pragma once
#include <TFT_eSPI.h>
#include "game_types.h"

// Draw the pet centered in the given sprite
// sprite should be sized to fit the drawing area
void drawPet(TFT_eSprite &spr, PetType type, uint8_t evoStage, PetMood mood,
             int cx, int cy, float breathScale);

// Draw a small pet preview icon (for selection screen)
void drawPetPreview(TFT_eSprite &spr, PetType type, int cx, int cy, int size);

// Get the primary color associated with a pet type
uint16_t petPrimaryColor(PetType type);

// Get mood-reactive eye/expression color
uint16_t petMoodColor(PetMood mood);
