#pragma once
#include "game_types.h"

// Initialize game state machine
void gameStateBegin();

// Main tick — call every frame with current input
void gameStateTick(const InputState &input);

// Get current UI state
UIState gameGetUIState();

// Get current tab
Tab gameGetCurrentTab();

// Force transition to a specific state
void gameSetUIState(UIState newState);

// Get current mini-game type
MiniGameType gameGetMiniGame();

// Check if a level-up popup should be shown
bool gameCheckLevelUp(uint16_t &outLevel);

// Get active mini game type for mini-game rendering
MiniGameType gameGetActiveMiniGame();
