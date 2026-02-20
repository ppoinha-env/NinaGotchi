#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>

// Initialize the audio player (DF1201S)
bool audio_begin();

// Non-blocking poll routine to be called in loop()
void audio_loop();

// Start playback of the specified file path (e.g. "/a.mp3")
// If force==false, do nothing when the same file is currently playing or in cooldown
bool audio_play(const char *path, bool force = false);

// Convenience wrapper that calls audio_play(path, true)
void audio_play_once(const char *path);

// Returns true if DF1201S reports a playback time > 0
bool audio_is_playing();

// Stop current playback
void audio_stop();

#endif // AUDIO_H
