#include "save_manager.h"
#include "pet.h"
#include "inventory.h"
#include <LittleFS.h>

static bool s_dirty = false;
static uint32_t s_lastSaveMs = 0;
static const uint32_t SAVE_DEBOUNCE_MS = 2000;

static DecayMode s_decayMode = DECAY_NORMAL;
static bool s_soundEnabled = true;

// ============================================================
// CRC32 (for save integrity)
// ============================================================

static uint32_t crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

// ============================================================
// Init
// ============================================================

static bool s_fsReady = false;

void saveManagerBegin() {
    s_dirty = false;
    s_lastSaveMs = millis();

    if (!LittleFS.begin(true)) {  // true = format on first use
        Serial.println("[Save] LittleFS mount failed");
        s_fsReady = false;
        return;
    }
    s_fsReady = true;

    // Create save directory if it doesn't exist
    if (!LittleFS.exists(SAVE_DIR)) {
        LittleFS.mkdir(SAVE_DIR);
    }
    Serial.println("[Save] LittleFS ready");
}

// ============================================================
// Load
// ============================================================

bool saveManagerLoad() {
    if (!s_fsReady) {
        Serial.println("[Save] LittleFS not ready, using defaults");
        return false;
    }

    File f = LittleFS.open(SAVE_FILE, FILE_READ);
    if (!f) {
        Serial.println("[Save] No save file found");
        return false;
    }

    SavePayload payload;
    size_t bytesRead = f.read((uint8_t*)&payload, sizeof(SavePayload));
    f.close();

    if (bytesRead != sizeof(SavePayload)) {
        Serial.printf("[Save] Bad size: %d vs %d\n", bytesRead, sizeof(SavePayload));
        return false;
    }

    if (payload.magic != SAVE_MAGIC) {
        Serial.println("[Save] Bad magic");
        return false;
    }

    if (payload.version != SAVE_VERSION) {
        Serial.println("[Save] Version mismatch");
        return false;
    }

    // Restore pet
    pet.fromPersist(payload.pet);

    // Restore inventory
    inventory.fromPersist(payload.inv);

    // Restore settings
    s_decayMode = (DecayMode)payload.decayMode;
    s_soundEnabled = (payload.soundEnabled != 0);

    Serial.printf("[Save] Loaded: %s (Lv%d %s)\n",
                  pet.getName(), pet.level, evoStageName((EvoStage)pet.evoStage));

    return true;
}

// ============================================================
// Save
// ============================================================

bool saveManagerSave() {
    if (!s_fsReady) return false;

    SavePayload payload;
    memset(&payload, 0, sizeof(payload));
    payload.magic = SAVE_MAGIC;
    payload.version = SAVE_VERSION;

    pet.toPersist(payload.pet);
    inventory.toPersist(payload.inv);
    payload.decayMode = (uint8_t)s_decayMode;
    payload.soundEnabled = s_soundEnabled ? 1 : 0;

    // Write to temp file first (atomic write)
    const char* tmpFile = "/rh_save/save.tmp";
    File f = LittleFS.open(tmpFile, FILE_WRITE);
    if (!f) {
        Serial.println("[Save] Failed to open tmp file");
        return false;
    }

    size_t written = f.write((uint8_t*)&payload, sizeof(SavePayload));
    f.close();

    if (written != sizeof(SavePayload)) {
        Serial.println("[Save] Write size mismatch");
        LittleFS.remove(tmpFile);
        return false;
    }

    // Rename tmp -> save
    LittleFS.remove(SAVE_FILE);
    LittleFS.rename(tmpFile, SAVE_FILE);

    s_dirty = false;
    s_lastSaveMs = millis();

    Serial.println("[Save] Saved OK");
    return true;
}

// ============================================================
// Dirty / Tick / Force
// ============================================================

void saveManagerMarkDirty() {
    s_dirty = true;
}

void saveManagerTick() {
    if (!s_dirty) return;
    if (millis() - s_lastSaveMs < SAVE_DEBOUNCE_MS) return;
    saveManagerSave();
}

void saveManagerForce() {
    s_dirty = true;
    saveManagerSave();
}

// ============================================================
// New Pet / Delete
// ============================================================

void saveManagerNewPet() {
    pet.init();
    inventory.init();
    s_decayMode = DECAY_NORMAL;
    saveManagerForce();
}

void saveManagerDeleteAll() {
    if (!s_fsReady) return;

    LittleFS.remove(SAVE_FILE);
    LittleFS.remove(SETTINGS_FILE);
    LittleFS.remove("/rh_save/save.tmp");

    Serial.println("[Save] All data deleted");
}

// ============================================================
// Decay Mode / Sound
// ============================================================

DecayMode saveManagerGetDecayMode() { return s_decayMode; }

void saveManagerSetDecayMode(DecayMode mode) {
    s_decayMode = mode;
    saveManagerMarkDirty();
}

bool saveManagerGetSoundEnabled() { return s_soundEnabled; }

void saveManagerSetSoundEnabled(bool enabled) {
    s_soundEnabled = enabled;
    saveManagerMarkDirty();
}
