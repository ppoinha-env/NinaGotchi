#pragma once
#include <stdint.h>

// ============================================================
// Raising Hell — CYD Port — Core Type Definitions
// ============================================================

// --- Pet Types (6) ---
enum PetType : uint8_t {
    PET_DEVIL = 0,
    PET_ELDRITCH,
    PET_ALIEN,
    PET_KAIJU,
    PET_ANUBIS,
    PET_AXOLOTL,
    PET_TYPE_COUNT
};

// --- Evolution Stages (4) ---
enum EvoStage : uint8_t {
    EVO_BABY = 0,
    EVO_TEEN,
    EVO_ADULT,
    EVO_ELDER,
    EVO_STAGE_COUNT
};

// --- Pet Moods (6) — resolved in priority order ---
enum PetMood : uint8_t {
    MOOD_SICK = 0,
    MOOD_TIRED,
    MOOD_HUNGRY,
    MOOD_MAD,
    MOOD_BORED,
    MOOD_HAPPY
};

// --- Item Types (5 + none) ---
enum ItemType : uint8_t {
    ITEM_NONE = 0,
    ITEM_SOUL_FOOD,
    ITEM_CURSED_RELIC,
    ITEM_DEMON_BONE,
    ITEM_RITUAL_CHALK,
    ITEM_ELDRITCH_EYE,
    ITEM_TYPE_COUNT
};

// --- UI States ---
enum UIState : uint8_t {
    UI_BOOT = 0,
    UI_CHOOSE_PET,
    UI_NAME_PET,
    UI_PET_SCREEN,
    UI_STATS,
    UI_FEED_MENU,
    UI_PLAY_MENU,
    UI_SLEEP_MENU,
    UI_INVENTORY,
    UI_SHOP,
    UI_SETTINGS,
    UI_DEATH,
    UI_BURIAL,
    UI_EVOLUTION,
    UI_MINI_GAME,
    UI_HATCHING,
    UI_LEVEL_UP
};

// --- Tab Bar ---
enum Tab : uint8_t {
    TAB_PET = 0,
    TAB_STATS,
    TAB_FEED,
    TAB_PLAY,
    TAB_SLEEP,
    TAB_INV,
    TAB_SHOP,
    TAB_COUNT
};

// --- Mini-Game Types ---
enum MiniGameType : uint8_t {
    MG_NONE = 0,
    MG_FLAPPY_FIREBALL,
    MG_CROSSY_HELL,
    MG_INFERNAL_DODGER,
    MG_RESURRECTION_RUN
};

// --- Decay Mode ---
enum DecayMode : uint8_t {
    DECAY_SUPER_SLOW = 0,   // 4x slower
    DECAY_SLOW,              // 2x slower
    DECAY_NORMAL,            // 1x
    DECAY_FAST,              // 2x faster
    DECAY_VERY_FAST,         // 4x faster
    DECAY_INSANE             // 32x faster
};

// --- Screen Layout ---
#define SCREEN_WIDTH      320
#define SCREEN_HEIGHT     240
#define TAB_BAR_HEIGHT    36
#define TAB_BAR_Y         (SCREEN_HEIGHT - TAB_BAR_HEIGHT)
#define CONTENT_HEIGHT    (SCREEN_HEIGHT - TAB_BAR_HEIGHT)
#define CONTENT_Y         0
#define HEADER_HEIGHT     28

// --- Game Theme Colors ---
#define COLOR_BG          0x0000  // Black
#define COLOR_HELLFIRE    0xF800  // Red
#define COLOR_LAVA        0xFB00  // Orange-red
#define COLOR_BRIMSTONE   0xFD20  // Orange
#define COLOR_SULFUR      0xFFE0  // Yellow
#define COLOR_SOUL        0x07FF  // Cyan
#define COLOR_INFERNAL    0x780F  // Purple
#define COLOR_BONE        0xDEDB  // Light gray
#define COLOR_ASH         0x4208  // Dark gray
#define COLOR_ECTOPLASM   0x07E0  // Green
#define COLOR_ELDRITCH    0x4810  // Dark green
#define COLOR_TAB_BG      0x18E3  // Dark gray tab bar
#define COLOR_TAB_ACTIVE  0xF800  // Red active tab
#define COLOR_TAB_TEXT    0xFFFF  // White
#define COLOR_HEALTH_BAR  0x07E0  // Green
#define COLOR_HUNGER_BAR  0xFD20  // Orange
#define COLOR_HAPPY_BAR   0xFFE0  // Yellow
#define COLOR_ENERGY_BAR  0x07FF  // Cyan
#define COLOR_GOLD        0xFEA0  // Gold for INF
#define COLOR_DARK_RED    0x8000  // Dark red
#define COLOR_MID_GRAY    0x7BEF  // Mid gray

// --- Pet Config ---
#define PET_NAME_MAX      12
#define MAX_INV_ITEMS     16

// --- Save Config ---
#define SAVE_MAGIC        0x52484353u  // 'RCHS'
#define SAVE_VERSION      1
#define SAVE_DIR          "/rh_save"
#define SAVE_FILE         "/rh_save/save.bin"
#define SETTINGS_FILE     "/rh_save/settings.bin"

// --- Decay Timing (Normal mode, in ms) ---
#define HUNGER_STEP_MS    864000UL    // 14.4 min — hunger -1
#define ENERGY_STEP_MS    576000UL    // 9.6 min  — energy -1
#define HP_STEP_MS        216000UL    // 3.6 min  — health -1 when starving
#define MOOD_HAPPY_MS     300000UL    // 5 min    — happiness -1 when happy
#define MOOD_DOWN_MS      30000UL     // 30s      — happiness -1 when not happy
#define HP_REGEN_MS       600000UL    // 10 min   — health +1 when fed & rested
#define SLEEP_TICK_MS     30000UL     // 30s      — sleep recovery tick

// --- XP / Level ---
#define XP_BASE           100
#define XP_PER_LEVEL      25
#define EVO_TEEN_LEVEL    5
#define EVO_ADULT_LEVEL   12
#define EVO_ELDER_LEVEL   25

// --- Touch Input ---
struct InputState {
    bool up;
    bool down;
    bool left;
    bool right;
    bool confirm;
    bool back;
    bool tabLeft;
    bool tabRight;
    int  tabDirect;   // -1 = none, 0-6 = direct tab tap
    bool anyTouch;
    int  touchX;
    int  touchY;
    bool freshPress;  // true only on first frame of touch
};

// --- Save Structures ---
struct PetPersist {
    uint8_t  hunger;
    uint8_t  happiness;
    uint8_t  energy;
    uint8_t  health;
    uint8_t  petType;
    uint8_t  isSleeping;
    uint8_t  evoStage;
    uint8_t  _pad;
    uint16_t level;
    uint32_t xp;
    int32_t  inf;
    uint32_t birthEpoch;
    char     name[PET_NAME_MAX + 1];
};

struct InvSlotPersist {
    uint8_t type;
    uint8_t qty;
};

struct InvPersist {
    InvSlotPersist slots[MAX_INV_ITEMS];
};

struct SavePayload {
    uint32_t   magic;
    uint16_t   version;
    uint16_t   _pad;
    PetPersist pet;
    InvPersist inv;
    uint8_t    decayMode;
    uint8_t    soundEnabled;
    uint8_t    _reserved[6];
};

// --- Inline Helpers ---
static inline int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Pet type name strings
static inline const char* petTypeName(PetType t) {
    switch (t) {
        case PET_DEVIL:    return "Devil";
        case PET_ELDRITCH: return "Eldritch";
        case PET_ALIEN:    return "Alien";
        case PET_KAIJU:    return "Kaiju";
        case PET_ANUBIS:   return "Anubis";
        case PET_AXOLOTL:  return "Axolotl";
        default:           return "Unknown";
    }
}

static inline const char* evoStageName(EvoStage s) {
    switch (s) {
        case EVO_BABY:  return "Baby";
        case EVO_TEEN:  return "Teen";
        case EVO_ADULT: return "Adult";
        case EVO_ELDER: return "Elder";
        default:        return "?";
    }
}

static inline const char* moodName(PetMood m) {
    switch (m) {
        case MOOD_SICK:    return "Sick";
        case MOOD_TIRED:   return "Tired";
        case MOOD_HUNGRY:  return "Hungry";
        case MOOD_MAD:     return "Mad";
        case MOOD_BORED:   return "Bored";
        case MOOD_HAPPY:   return "Happy";
        default:           return "?";
    }
}

// Decay mode multiplier (returns fixed-point Q8: 256 = 1.0x)
static inline uint16_t decayMultiplierQ8(DecayMode m) {
    switch (m) {
        case DECAY_SUPER_SLOW: return 64;    // 0.25x (4x slower)
        case DECAY_SLOW:       return 128;   // 0.5x  (2x slower)
        case DECAY_NORMAL:     return 256;   // 1.0x
        case DECAY_FAST:       return 512;   // 2.0x  (2x faster)
        case DECAY_VERY_FAST:  return 1024;  // 4.0x  (4x faster)
        case DECAY_INSANE:     return 8192;  // 32.0x
        default:               return 256;
    }
}
