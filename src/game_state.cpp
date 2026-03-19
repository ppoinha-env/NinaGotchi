#include "game_state.h"
#include "pet.h"
#include "inventory.h"
#include "shop.h"
#include "save_manager.h"
#include "ui_renderer.h"
#include "mini_games.h"
#include "audio.h"

// ============================================================
// State Variables
// ============================================================

static UIState s_uiState = UI_BOOT;
static Tab     s_currentTab = TAB_PET;

// Menu indices
static int s_choosePetIdx = 0;
static int s_feedIdx = 0;
static int s_playIdx = 0;
static int s_sleepIdx = 0;
static int s_invIdx = 0;
static int s_shopIdx = 0;
static int s_deathIdx = 0;
static int s_settingsIdx = 0;

// Name entry state
static char s_nameBuffer[PET_NAME_MAX + 1] = "";
static int  s_nameCursor = 0;
static int  s_kbRow = 0;
static int  s_kbCol = 0;

// Hatching animation
static int s_hatchFrame = 0;
static uint32_t s_hatchStartMs = 0;

// Evolution animation
static int s_evoPhase = 0;
static uint32_t s_evoStartMs = 0;
static uint8_t s_evoFromStage = 0;
static uint8_t s_evoToStage = 0;

// Level-up popup
static bool s_levelUpPending = false;
static uint16_t s_levelUpLevel = 0;
static uint16_t s_prevLevel = 1;
static uint32_t s_levelUpShowMs = 0;

// Mini-game
static MiniGameType s_activeMiniGame = MG_NONE;

// Sleep timer
static uint32_t s_lastSleepTickMs = 0;

// Input debounce for menus
static uint32_t s_lastInputMs = 0;
static const uint32_t INPUT_DEBOUNCE_MS = 200;

// Needs full redraw
static bool s_needsRedraw = true;

// ============================================================
// Helpers
// ============================================================

static bool inputDebounced() {
    if (millis() - s_lastInputMs < INPUT_DEBOUNCE_MS) return false;
    s_lastInputMs = millis();
    return true;
}

// Detect which menu item was directly tapped.
// menuStartY: Y of first item, itemH: height per item (including gap),
// itemCount: total items. Returns -1 if no item was hit.
static int hitTestMenuItem(const InputState &input, int menuStartY, int itemH, int itemCount) {
    if (!input.anyTouch || !input.freshPress) return -1;
    if (input.touchY < menuStartY || input.touchY >= TAB_BAR_Y) return -1;
    int idx = (input.touchY - menuStartY) / itemH;
    if (idx < 0 || idx >= itemCount) return -1;
    return idx;
}

static void transitionTo(UIState state) {
    s_uiState = state;
    s_needsRedraw = true;
}

// ============================================================
// Init
// ============================================================

void gameStateBegin() {
    s_uiState = UI_BOOT;
    s_currentTab = TAB_PET;
    s_needsRedraw = true;
    s_prevLevel = pet.level;
    s_lastSleepTickMs = millis();
}

UIState  gameGetUIState()       { return s_uiState; }
Tab      gameGetCurrentTab()    { return s_currentTab; }
MiniGameType gameGetMiniGame()  { return s_activeMiniGame; }
MiniGameType gameGetActiveMiniGame() { return s_activeMiniGame; }

void gameSetUIState(UIState newState) { transitionTo(newState); }

bool gameCheckLevelUp(uint16_t &outLevel) {
    if (s_levelUpPending) {
        outLevel = s_levelUpLevel;
        if (millis() - s_levelUpShowMs > 2000) {
            s_levelUpPending = false;
        }
        return true;
    }
    return false;
}

// ============================================================
// Tab navigation helper
// ============================================================

static UIState tabToUIState(Tab tab) {
    switch (tab) {
        case TAB_PET:   return UI_PET_SCREEN;
        case TAB_STATS: return UI_STATS;
        case TAB_FEED:  return UI_FEED_MENU;
        case TAB_PLAY:  return UI_PLAY_MENU;
        case TAB_SLEEP: return UI_SLEEP_MENU;
        case TAB_INV:   return UI_INVENTORY;
        case TAB_SHOP:  return UI_SHOP;
        default:        return UI_PET_SCREEN;
    }
}

static void switchTab(Tab newTab) {
    s_currentTab = newTab;
    transitionTo(tabToUIState(newTab));
}

// ============================================================
// Handle tab input (shared across states)
// ============================================================

static bool handleTabInput(const InputState &input) {
    if (input.tabDirect >= 0 && input.tabDirect < TAB_COUNT && input.freshPress) {
        Tab newTab = (Tab)input.tabDirect;
        if (newTab != s_currentTab) {
            switchTab(newTab);
            return true;
        }
    }
    return false;
}

// ============================================================
// State: BOOT
// ============================================================

static void tickBoot(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawBootScreen();
        s_needsRedraw = false;
    }
    if (input.anyTouch && input.freshPress) {
        // Try loading save
        if (saveManagerLoad()) {
            // Save found — go to pet screen
            s_prevLevel = pet.level;
            switchTab(TAB_PET);
        } else {
            // No save — choose pet
            transitionTo(UI_CHOOSE_PET);
        }
        audio_play("/henina.mp3");
    }
}

// ============================================================
// State: CHOOSE_PET
// ============================================================

static void tickChoosePet(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawChoosePetScreen(s_choosePetIdx);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;
    if (handleTabInput(input)) return;

    // Detect tap on pet grid
    if (input.anyTouch && input.touchY < TAB_BAR_Y && input.touchY > HEADER_HEIGHT) {
        int colW = SCREEN_WIDTH / 3;
        int rowH = (CONTENT_HEIGHT - HEADER_HEIGHT) / 2;
        int col = input.touchX / colW;
        int row = (input.touchY - HEADER_HEIGHT) / rowH;
        int idx = row * 3 + col;
        if (idx >= 0 && idx < PET_TYPE_COUNT) {
            if (idx == s_choosePetIdx) {
                // Double tap = confirm
                pet.init();
                pet.type = (PetType)s_choosePetIdx;
                s_nameCursor = 0;
                memset(s_nameBuffer, 0, sizeof(s_nameBuffer));
                s_kbRow = 0;
                s_kbCol = 0;
                transitionTo(UI_NAME_PET);
                audio_play("/a.mp3");
            } else {
                s_choosePetIdx = idx;
                s_needsRedraw = true;
            }
        }
    }
}

// ============================================================
// State: NAME_PET
// ============================================================

#define KB_ROWS_COUNT 3
#define KB_COLS_COUNT 12
static const char KB_CHARS[][13] = {
    "ABCDEFGHIJKL",
    "MNOPQRSTUVWX",
    "YZ0123456789",
};
#define KB_START_X ((SCREEN_WIDTH - KB_COLS_COUNT * 24) / 2)
#define KB_START_Y (HEADER_HEIGHT + 45)
#define KB_KEY_SZ_W 24
#define KB_KEY_SZ_H 28
#define KB_ACTION_Y (KB_START_Y + KB_ROWS_COUNT * KB_KEY_SZ_H + 5)

static void tickNamePet(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawNameEntryScreen(s_nameBuffer, s_nameCursor, s_kbRow, s_kbCol);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;

    // Detect keyboard tap
    if (input.anyTouch && input.touchY >= KB_START_Y && input.touchY < KB_ACTION_Y) {
        int col = (input.touchX - KB_START_X) / KB_KEY_SZ_W;
        int row = (input.touchY - KB_START_Y) / KB_KEY_SZ_H;
        if (row >= 0 && row < KB_ROWS_COUNT && col >= 0 && col < KB_COLS_COUNT) {
            s_kbRow = row;
            s_kbCol = col;
            char ch = KB_CHARS[row][col];
            if (ch != '\0' && s_nameCursor < PET_NAME_MAX) {
                s_nameBuffer[s_nameCursor] = ch;
                s_nameCursor++;
                s_nameBuffer[s_nameCursor] = '\0';
                audio_play("/a.mp3");
            }
        }
        s_needsRedraw = true;
    }
    // DEL button area
    else if (input.anyTouch && input.touchY >= KB_ACTION_Y &&
             input.touchX < SCREEN_WIDTH / 2) {
        if (s_nameCursor > 0) {
            s_nameCursor--;
            s_nameBuffer[s_nameCursor] = '\0';
        }
        s_needsRedraw = true;
    }
    // OK button area
    else if (input.anyTouch && input.touchY >= KB_ACTION_Y &&
             input.touchX >= SCREEN_WIDTH / 2) {
        if (s_nameCursor > 0) {
            pet.setName(s_nameBuffer);
            s_hatchFrame = 0;
            s_hatchStartMs = millis();
            transitionTo(UI_HATCHING);
            audio_play("/henina.mp3");
        }
    }
}

// ============================================================
// State: HATCHING
// ============================================================

static void tickHatching(const InputState &input) {
    uint32_t elapsed = millis() - s_hatchStartMs;
    s_hatchFrame = elapsed / 50;  // ~20fps animation

    uiDrawHatchingScreen(pet.type, s_hatchFrame);

    if (s_hatchFrame > 80) {
        // Hatching complete — save and go to pet screen
        saveManagerForce();
        switchTab(TAB_PET);
    }
}

// ============================================================
// State: PET_SCREEN
// ============================================================

static void tickPetScreen(const InputState &input) {
    uiDrawPetScreen();
    uiDrawTabBar(TAB_PET);

    if (!input.freshPress) return;
    if (handleTabInput(input)) return;

    // Long press center area → settings
    if (input.back) {
        transitionTo(UI_SETTINGS);
    }
}

// ============================================================
// State: STATS
// ============================================================

static void tickStats(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawStatsScreen();
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;
    if (handleTabInput(input)) return;
    if (input.back) switchTab(TAB_PET);
}

// ============================================================
// State: FEED_MENU
// ============================================================

static void tickFeedMenu(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawFeedMenu(s_feedIdx);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;
    if (handleTabInput(input)) return;

    int visCount = inventory.getVisibleCount();

    // Direct tap on menu item
    int tapped = hitTestMenuItem(input, HEADER_HEIGHT + 4, 30, visCount);
    if (tapped >= 0) {
        if (tapped == s_feedIdx) {
            // Already selected — confirm
            input.confirm ? (void)0 : (void)0; // fall through to confirm logic below
        } else {
            s_feedIdx = tapped;
            s_needsRedraw = true;
            return;
        }
    }

    if (input.up && inputDebounced()) {
        s_feedIdx = (s_feedIdx - 1 + visCount) % max(visCount, 1);
        s_needsRedraw = true;
    }
    if (input.down && inputDebounced()) {
        s_feedIdx = (s_feedIdx + 1) % max(visCount, 1);
        s_needsRedraw = true;
    }
    if ((input.confirm || tapped == s_feedIdx) && visCount > 0) {
        inventory.selectedIndex = s_feedIdx;
        ItemType type = inventory.getVisibleType(s_feedIdx);
        if (type == ITEM_ELDRITCH_EYE && pet.canEvolve()) {
            inventory.removeItem(ITEM_ELDRITCH_EYE, 1);
            s_evoFromStage = pet.evoStage;
            s_evoToStage = pet.evoStage + 1;
            s_evoPhase = 0;
            s_evoStartMs = millis();
            transitionTo(UI_EVOLUTION);
            audio_play("/henina.mp3");
        } else if (inventory.useSelectedItem()) {
            audio_play("/yumtasty.mp3");
            saveManagerMarkDirty();
            s_needsRedraw = true;
        } else {
            audio_play("/awishap.mp3");
        }
    }
    if (input.back) switchTab(TAB_PET);
}

// ============================================================
// State: PLAY_MENU
// ============================================================

static void tickPlayMenu(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawPlayMenu(s_playIdx);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;
    if (handleTabInput(input)) return;

    // Direct tap on menu item
    int tapped = hitTestMenuItem(input, HEADER_HEIGHT + 8, 28, 4);
    if (tapped >= 0) {
        if (tapped != s_playIdx) {
            s_playIdx = tapped;
            s_needsRedraw = true;
            return;
        }
        // tapped == s_playIdx → fall through to confirm
    }

    if (input.up && inputDebounced()) {
        s_playIdx = (s_playIdx - 1 + 4) % 4;
        s_needsRedraw = true;
    }
    if (input.down && inputDebounced()) {
        s_playIdx = (s_playIdx + 1) % 4;
        s_needsRedraw = true;
    }
    if (input.confirm || tapped == s_playIdx) {
        // Check if game is available
        if (s_playIdx == 3 && !pet.isDead()) {
            audio_play("/awishap.mp3");
            return;
        }
        // Check energy
        if (pet.energy < 5 && !pet.isDead()) {
            audio_play("/awishap.mp3");
            return;
        }
        if (!pet.isDead()) pet.tire(5);

        // Start mini-game
        MiniGameType mg = MG_NONE;
        switch (s_playIdx) {
            case 0: mg = MG_FLAPPY_FIREBALL; break;
            case 1: mg = MG_CROSSY_HELL; break;
            case 2: mg = MG_INFERNAL_DODGER; break;
            case 3: mg = MG_RESURRECTION_RUN; break;
        }
        s_activeMiniGame = mg;
        miniGameStart(mg);
        transitionTo(UI_MINI_GAME);
        audio_play("/a.mp3");
    }
    if (input.back) switchTab(TAB_PET);
}

// ============================================================
// State: SLEEP_MENU
// ============================================================

static void tickSleepMenu(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawSleepMenu(s_sleepIdx);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;
    if (handleTabInput(input)) return;

    // Direct tap on menu item
    int tapped = hitTestMenuItem(input, HEADER_HEIGHT + 30, 28, 4);
    if (tapped >= 0) {
        if (tapped != s_sleepIdx) {
            s_sleepIdx = tapped;
            s_needsRedraw = true;
            return;
        }
        // tapped == s_sleepIdx → fall through to confirm
    }

    if (input.up && inputDebounced()) {
        s_sleepIdx = (s_sleepIdx - 1 + 4) % 4;
        s_needsRedraw = true;
    }
    if (input.down && inputDebounced()) {
        s_sleepIdx = (s_sleepIdx + 1) % 4;
        s_needsRedraw = true;
    }
    if (input.confirm || tapped == s_sleepIdx) {
        switch (s_sleepIdx) {
            case 0: // Quick nap
                pet.startSleep();
                audio_play("/onorain.mp3");
                break;
            case 1: // Full rest
                pet.startSleep();
                audio_play("/onorain.mp3");
                break;
            case 2: // Light sleep
                pet.startSleep();
                audio_play("/onorain.mp3");
                break;
            case 3: // Cancel
                switchTab(TAB_PET);
                return;
        }
        s_needsRedraw = true;
        saveManagerMarkDirty();
    }
    if (input.back) switchTab(TAB_PET);

    // If pet is sleeping and user taps, wake pet
    if (pet.isSleeping && input.anyTouch && input.freshPress &&
        input.touchY < TAB_BAR_Y && input.touchY > HEADER_HEIGHT + 60) {
        pet.wakeUp();
        s_needsRedraw = true;
        saveManagerMarkDirty();
    }
}

// ============================================================
// State: INVENTORY
// ============================================================

static void tickInventory(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawInventoryScreen(s_invIdx);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;
    if (handleTabInput(input)) return;

    int visCount = inventory.getVisibleCount();

    // Direct tap on menu item
    int tapped = hitTestMenuItem(input, HEADER_HEIGHT + 4, 30, visCount);
    if (tapped >= 0) {
        if (tapped != s_invIdx) {
            s_invIdx = tapped;
            s_needsRedraw = true;
            return;
        }
    }

    if (input.up && inputDebounced()) {
        s_invIdx = (s_invIdx - 1 + max(visCount, 1)) % max(visCount, 1);
        s_needsRedraw = true;
    }
    if (input.down && inputDebounced()) {
        s_invIdx = (s_invIdx + 1) % max(visCount, 1);
        s_needsRedraw = true;
    }
    if ((input.confirm || tapped == s_invIdx) && visCount > 0) {
        inventory.selectedIndex = s_invIdx;
        ItemType type = inventory.getVisibleType(s_invIdx);
        if (type == ITEM_ELDRITCH_EYE && pet.canEvolve()) {
            inventory.removeItem(ITEM_ELDRITCH_EYE, 1);
            s_evoFromStage = pet.evoStage;
            s_evoToStage = pet.evoStage + 1;
            s_evoPhase = 0;
            s_evoStartMs = millis();
            transitionTo(UI_EVOLUTION);
            audio_play("/henina.mp3");
        } else if (inventory.useSelectedItem()) {
            audio_play("/yumtasty.mp3");
            saveManagerMarkDirty();
            s_needsRedraw = true;
        } else {
            audio_play("/awishap.mp3");
        }
    }
    if (input.back) switchTab(TAB_PET);
}

// ============================================================
// State: SHOP
// ============================================================

static void tickShop(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawShopScreen(s_shopIdx);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;
    if (handleTabInput(input)) return;

    // Direct tap on menu item
    int tapped = hitTestMenuItem(input, HEADER_HEIGHT + 4, 30, SHOP_ITEM_COUNT);
    if (tapped >= 0) {
        if (tapped != s_shopIdx) {
            s_shopIdx = tapped;
            s_needsRedraw = true;
            return;
        }
    }

    if (input.up && inputDebounced()) {
        s_shopIdx = (s_shopIdx - 1 + SHOP_ITEM_COUNT) % SHOP_ITEM_COUNT;
        s_needsRedraw = true;
    }
    if (input.down && inputDebounced()) {
        s_shopIdx = (s_shopIdx + 1) % SHOP_ITEM_COUNT;
        s_needsRedraw = true;
    }
    if (input.confirm || tapped == s_shopIdx) {
        if (shopBuyItem(s_shopIdx)) {
            audio_play("/yumtasty.mp3");
            saveManagerMarkDirty();
        } else {
            audio_play("/awishap.mp3");
        }
        s_needsRedraw = true;
    }
    if (input.back) switchTab(TAB_PET);
}

// ============================================================
// State: DEATH
// ============================================================

static void tickDeath(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawDeathScreen(s_deathIdx);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;

    // Direct tap on menu item
    int tapped = hitTestMenuItem(input, 130, 28, 2);
    if (tapped >= 0) {
        if (tapped != s_deathIdx) {
            s_deathIdx = tapped;
            s_needsRedraw = true;
            return;
        }
    }

    if (input.up || input.down) {
        s_deathIdx = 1 - s_deathIdx;
        s_needsRedraw = true;
    }
    if (input.confirm || tapped == s_deathIdx) {
        if (s_deathIdx == 0) {
            // Resurrection Run
            s_activeMiniGame = MG_RESURRECTION_RUN;
            miniGameStart(MG_RESURRECTION_RUN);
            transitionTo(UI_MINI_GAME);
        } else {
            // Bury and start new
            transitionTo(UI_BURIAL);
        }
    }
}

// ============================================================
// State: BURIAL
// ============================================================

static void tickBurial(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawBurialScreen();
        s_needsRedraw = false;
    }

    if (input.anyTouch && input.freshPress) {
        // Delete save and start fresh
        saveManagerDeleteAll();
        pet.init();
        inventory.init();
        s_choosePetIdx = 0;
        transitionTo(UI_CHOOSE_PET);
    }
}

// ============================================================
// State: EVOLUTION
// ============================================================

static void tickEvolution(const InputState &input) {
    uint32_t elapsed = millis() - s_evoStartMs;

    if (elapsed < 1500) {
        s_evoPhase = 0;
    } else if (elapsed < 1700) {
        s_evoPhase = 1;
    } else if (elapsed < 3500) {
        s_evoPhase = 2;
    } else {
        // Complete — apply evolution
        pet.setEvoStage(s_evoToStage);
        saveManagerMarkDirty();
        switchTab(TAB_PET);
        audio_play("/ailunina.mp3");
        return;
    }

    uiDrawEvolutionScreen(s_evoPhase, s_evoFromStage, s_evoToStage);
}

// ============================================================
// State: MINI_GAME
// ============================================================

static void tickMiniGame(const InputState &input) {
    miniGameTick(input);

    // Check if mini-game has ended AND result screen has been dismissed
    MiniGameResult result = miniGameGetResult();
    if (result != MG_RESULT_PLAYING) {
        // Wait for result screen to be dismissed (touch after 1s)
        // miniGameTick handles showing the result screen.
        // Only transition once user taps to acknowledge.
        if (!input.freshPress) return;
        // Need at least 1s on result screen — handled inside miniGameTick

        // Apply rewards
        if (result == MG_RESULT_WIN) {
            if (s_activeMiniGame == MG_RESURRECTION_RUN) {
                // Resurrect pet
                pet.health = 50;
                pet.hunger = 50;
                pet.energy = 30;
                pet.happiness = 50;
                audio_play("/ailunina.mp3");
            } else {
                // Normal win rewards
                int xpReward = 15 + random(0, 20);
                int infReward = 5 + random(0, 15);
                pet.addXP(xpReward);
                addInf(infReward);
                pet.makeHappy(10);
                audio_play("/ailunina.mp3");
            }
        } else {
            // Loss
            if (s_activeMiniGame == MG_RESURRECTION_RUN) {
                // Failed resurrection — stay dead
                audio_play("/onorain.mp3");
            } else {
                pet.makeHappy(3);  // Participation bonus
                audio_play("/awishap.mp3");
            }
        }

        saveManagerMarkDirty();
        s_activeMiniGame = MG_NONE;

        // Return to appropriate screen
        if (pet.isDead()) {
            s_deathIdx = 0;
            transitionTo(UI_DEATH);
        } else {
            switchTab(TAB_PET);
        }
    }
}

// ============================================================
// State: SETTINGS
// ============================================================

static void tickSettings(const InputState &input) {
    if (s_needsRedraw) {
        uiDrawSettingsScreen(s_settingsIdx);
        s_needsRedraw = false;
    }

    if (!input.freshPress) return;

    // Direct tap on menu item
    int tapped = hitTestMenuItem(input, HEADER_HEIGHT + 8, 28, 4);
    if (tapped >= 0) {
        if (tapped != s_settingsIdx) {
            s_settingsIdx = tapped;
            s_needsRedraw = true;
            return;
        }
    }

    if (input.up && inputDebounced()) {
        s_settingsIdx = (s_settingsIdx - 1 + 4) % 4;
        s_needsRedraw = true;
    }
    if (input.down && inputDebounced()) {
        s_settingsIdx = (s_settingsIdx + 1) % 4;
        s_needsRedraw = true;
    }
    if (input.confirm || tapped == s_settingsIdx) {
        switch (s_settingsIdx) {
            case 0: {
                // Cycle decay mode
                int dm = (int)saveManagerGetDecayMode();
                dm = (dm + 1) % 6;
                saveManagerSetDecayMode((DecayMode)dm);
                s_needsRedraw = true;
                break;
            }
            case 1:
                // Toggle sound
                saveManagerSetSoundEnabled(!saveManagerGetSoundEnabled());
                s_needsRedraw = true;
                break;
            case 2:
                // Factory reset
                saveManagerDeleteAll();
                pet.init();
                inventory.init();
                transitionTo(UI_BOOT);
                break;
            case 3:
                // Back
                switchTab(TAB_PET);
                break;
        }
    }
    if (input.back) switchTab(TAB_PET);
}

// ============================================================
// Background Services (called every tick regardless of state)
// ============================================================

static void backgroundServices() {
    DecayMode dm = saveManagerGetDecayMode();

    // Pet stat decay (when not in mini-game and not dead)
    if (s_uiState != UI_MINI_GAME && s_uiState != UI_BOOT &&
        s_uiState != UI_CHOOSE_PET && s_uiState != UI_NAME_PET &&
        s_uiState != UI_HATCHING && !pet.isDead()) {

        pet.update(dm);

        // Sleep ticks
        if (pet.isSleeping) {
            if (millis() - s_lastSleepTickMs >= SLEEP_TICK_MS) {
                s_lastSleepTickMs = millis();
                pet.sleepTick();
                saveManagerMarkDirty();
            }
        }

        // Death check
        if (pet.isDead() && s_uiState != UI_DEATH && s_uiState != UI_BURIAL) {
            s_deathIdx = 0;
            transitionTo(UI_DEATH);
            audio_play("/onorain.mp3");
        }
    }

    // Level-up detection
    if (pet.level > s_prevLevel) {
        s_levelUpPending = true;
        s_levelUpLevel = pet.level;
        s_levelUpShowMs = millis();
        s_prevLevel = pet.level;
        audio_play("/ailunina.mp3");

        // Check evolution eligibility notification
        if (pet.canEvolve()) {
            // Could add visual notification here
        }
    }

    // Save manager tick
    saveManagerTick();
}

// ============================================================
// Main Dispatch
// ============================================================

void gameStateTick(const InputState &input) {
    backgroundServices();

    switch (s_uiState) {
        case UI_BOOT:        tickBoot(input);       break;
        case UI_CHOOSE_PET:  tickChoosePet(input);  break;
        case UI_NAME_PET:    tickNamePet(input);    break;
        case UI_HATCHING:    tickHatching(input);   break;
        case UI_PET_SCREEN:  tickPetScreen(input);  break;
        case UI_STATS:       tickStats(input);      break;
        case UI_FEED_MENU:   tickFeedMenu(input);   break;
        case UI_PLAY_MENU:   tickPlayMenu(input);   break;
        case UI_SLEEP_MENU:  tickSleepMenu(input);  break;
        case UI_INVENTORY:   tickInventory(input);  break;
        case UI_SHOP:        tickShop(input);       break;
        case UI_DEATH:       tickDeath(input);      break;
        case UI_BURIAL:      tickBurial(input);     break;
        case UI_EVOLUTION:   tickEvolution(input);  break;
        case UI_MINI_GAME:   tickMiniGame(input);   break;
        case UI_SETTINGS:    tickSettings(input);   break;
        default:
            transitionTo(UI_BOOT);
            break;
    }

    // Level-up popup overlay
    uint16_t lvl;
    if (gameCheckLevelUp(lvl)) {
        uiDrawLevelUpPopup(lvl);
    }
}
