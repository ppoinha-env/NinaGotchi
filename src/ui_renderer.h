#pragma once
#include <TFT_eSPI.h>
#include "game_types.h"

// Forward decl — tft & sprite are owned by main.cpp
extern TFT_eSPI tft;
extern TFT_eSprite mainSprite;

// --- Screen sections ---
void uiDrawTabBar(Tab activeTab);
void uiDrawHeader(const char* title, bool showBack = false);
void uiClearContent();

// --- Full screens ---
void uiDrawBootScreen();
void uiDrawChoosePetScreen(int selectedIndex);
void uiDrawNameEntryScreen(const char* currentName, int cursorPos, int selectedRow, int selectedCol);
void uiDrawHatchingScreen(PetType type, int frame);
void uiDrawPetScreen();
void uiDrawStatsScreen();
void uiDrawFeedMenu(int selectedIndex);
void uiDrawPlayMenu(int selectedIndex);
void uiDrawSleepMenu(int selectedIndex);
void uiDrawInventoryScreen(int selectedIndex);
void uiDrawShopScreen(int selectedIndex);
void uiDrawDeathScreen(int selectedIndex);
void uiDrawBurialScreen();
void uiDrawEvolutionScreen(int phase, uint8_t fromStage, uint8_t toStage);
void uiDrawLevelUpPopup(uint16_t newLevel);
void uiDrawSettingsScreen(int selectedIndex);

// --- Helpers ---
void uiDrawStatBar(int x, int y, int w, int h, int value, int maxVal, uint16_t color, const char* label);
void uiDrawMenuItem(int y, const char* text, bool selected, uint16_t color = TFT_WHITE);
void uiDrawCenteredText(const char* text, int y, uint16_t color, int textSize = 2);
