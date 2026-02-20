#include "audio.h"
#include <DFRobot_DF1201S.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static DFRobot_DF1201S DF1201S;

static String lastPlayedPath = "";
static String lastQueuedPath = "";
static unsigned long lastPlayMs = 0;
static unsigned long lastQueueMs = 0;
static const unsigned long COOLDOWN_MS = 1200;
static volatile bool isCurrentlyPlaying = false;
static volatile bool isInitialized = false;

enum AudioCommandType { AUDIO_CMD_PLAY, AUDIO_CMD_STOP };

struct AudioCommand {
    AudioCommandType type;
    char path[64];
    bool force;
};

static QueueHandle_t audioQueue = nullptr;
static TaskHandle_t audioTaskHandle = nullptr;

static void audio_task(void *parameter) {
    (void)parameter;

    unsigned long lastPollMs = 0;
    AudioCommand cmd;

    while (true) {
        if (xQueueReceive(audioQueue, &cmd, pdMS_TO_TICKS(20)) == pdTRUE) {
            if (cmd.type == AUDIO_CMD_STOP) {
                DF1201S.pause();
                isCurrentlyPlaying = false;
            } else if (cmd.type == AUDIO_CMD_PLAY) {
                String requestedPath = String(cmd.path);
                unsigned long now = millis();

                if (!cmd.force) {
                    if (requestedPath == lastPlayedPath && (now - lastPlayMs < COOLDOWN_MS)) {
                        continue;
                    }
                }

                bool started = DF1201S.playSpecFile(requestedPath);
                if (!started && requestedPath.startsWith("/")) {
                    String fallbackPath = requestedPath.substring(1);
                    started = DF1201S.playSpecFile(fallbackPath);
                    if (started) {
                        requestedPath = fallbackPath;
                    }
                }

                if (started) {
                    lastPlayedPath = requestedPath;
                    lastPlayMs = now;
                    isCurrentlyPlaying = true;
                }
            }
        }

        unsigned long now = millis();
        if (now - lastPollMs > 200) {
            lastPollMs = now;
            isCurrentlyPlaying = DF1201S.isPlaying();
        }
    }
}

bool audio_begin() {
    Serial2.begin(115200, SERIAL_8N1, 27, 22);
    
    // Try to initialize the DFPlayer
    int retries = 5;
    bool success = false;
    while (retries > 0) {
        if (DF1201S.begin(Serial2)) {
            success = true;
            break;
        }
        Serial.println("Waiting for DFPlayer...");
        delay(500);
        retries--;
    }

    if (success) {
        DF1201S.setPrompt(false);
        DF1201S.setVol(20);
        DF1201S.switchFunction(DF1201S.MUSIC);
        DF1201S.setPlayMode(DF1201S.SINGLE);
        delay(1000); // Give it a moment to settle
        DF1201S.pause(); // Ensure no previous auto-play continues
        isInitialized = true;

        if (audioQueue == nullptr) {
            audioQueue = xQueueCreate(8, sizeof(AudioCommand));
        }

        if (audioQueue != nullptr && audioTaskHandle == nullptr) {
            xTaskCreatePinnedToCore(
                audio_task,
                "audio_task",
                4096,
                nullptr,
                1,
                &audioTaskHandle,
                0
            );
        }

        Serial.println("DFPlayer initialized successfully.");
    } else {
        isInitialized = false;
        Serial.println("DFPlayer initialization failed.");
    }
    
    return success;
}

void audio_loop() {
    // Playback and state polling are handled in audio_task() on Core 0.
}

bool audio_play(const char *path, bool force) {
    if (!isInitialized || path == nullptr || path[0] == '\0') return false;

    if (audioQueue == nullptr) return false;

    unsigned long now = millis();
    String requestedPath = String(path);

    if (!force) {
        if (lastQueuedPath == requestedPath && (now - lastQueueMs < COOLDOWN_MS)) {
            return false;
        }
    }

    AudioCommand cmd;
    cmd.type = AUDIO_CMD_PLAY;
    cmd.force = force;
    requestedPath.toCharArray(cmd.path, sizeof(cmd.path));

    if (xQueueSend(audioQueue, &cmd, 0) != pdTRUE) {
        AudioCommand dropped;
        xQueueReceive(audioQueue, &dropped, 0);
        if (xQueueSend(audioQueue, &cmd, 0) != pdTRUE) {
            return false;
        }
    }

    lastQueuedPath = requestedPath;
    lastQueueMs = now;

    Serial.print("Queued audio: ");
    Serial.println(requestedPath);

    return true;
}

void audio_play_once(const char *path) {
    audio_play(path, true);
}

bool audio_is_playing() {
    return isInitialized && isCurrentlyPlaying;
}

void audio_stop() {
    if (!isInitialized || audioQueue == nullptr) return;

    AudioCommand cmd;
    cmd.type = AUDIO_CMD_STOP;
    cmd.path[0] = '\0';
    cmd.force = true;

    if (xQueueSend(audioQueue, &cmd, 0) != pdTRUE) {
        AudioCommand dropped;
        xQueueReceive(audioQueue, &dropped, 0);
        xQueueSend(audioQueue, &cmd, 0);
    }
}
