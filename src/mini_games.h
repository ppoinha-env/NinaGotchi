#pragma once
#include "game_types.h"

// Mini-game result
enum MiniGameResult : uint8_t {
    MG_RESULT_PLAYING = 0,
    MG_RESULT_WIN,
    MG_RESULT_LOSE
};

// Start a mini-game
void miniGameStart(MiniGameType type);

// Tick the active mini-game (update + draw)
void miniGameTick(const InputState &input);

// Get current result
MiniGameResult miniGameGetResult();
