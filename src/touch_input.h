#pragma once
#include <Arduino.h>
#include <XPT2046_Touchscreen.h>
#include "game_types.h"

// Initialize the touch input system (call after touch.begin())
void touchInputBegin();

// Poll touch and produce an InputState for this frame
// Call once per frame in the main loop
InputState touchInputPoll(XPT2046_Touchscreen &touch);

// Get raw mapped coordinates from last poll
int touchLastX();
int touchLastY();

// Get time since last touch event
unsigned long touchIdleMs();

// Reset idle timer (e.g., after wake)
void touchResetIdle();
