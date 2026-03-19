#include "ui_renderer.h"
#include "pet.h"
#include "pet_visuals.h"
#include "inventory.h"
#include "shop.h"
#include "save_manager.h"

// ============================================================
// Tab Bar
// ============================================================

static const char* tabLabels[] = { "Pet", "Stat", "Feed", "Play", "Slp", "Inv", "Shop" };

void uiDrawTabBar(Tab activeTab) {
    int tabW = SCREEN_WIDTH / TAB_COUNT;
    for (int i = 0; i < TAB_COUNT; i++) {
        int x = i * tabW;
        bool active = (i == (int)activeTab);
        uint16_t bg = active ? COLOR_TAB_ACTIVE : COLOR_TAB_BG;
        uint16_t fg = active ? TFT_BLACK : COLOR_TAB_TEXT;

        tft.fillRect(x, TAB_BAR_Y, tabW, TAB_BAR_HEIGHT, bg);
        tft.drawRect(x, TAB_BAR_Y, tabW, TAB_BAR_HEIGHT, COLOR_ASH);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(fg, bg);
        tft.setTextSize(1);
        tft.drawString(tabLabels[i], x + tabW / 2, TAB_BAR_Y + TAB_BAR_HEIGHT / 2);
    }
}

// ============================================================
// Header & Content Clear
// ============================================================

void uiDrawHeader(const char* title, bool showBack) {
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_DARK_RED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, COLOR_DARK_RED);
    tft.setTextSize(1);
    tft.drawString(title, SCREEN_WIDTH / 2, HEADER_HEIGHT / 2 + 1);

    if (showBack) {
        tft.setTextDatum(ML_DATUM);
        tft.setTextColor(COLOR_SULFUR, COLOR_DARK_RED);
        tft.drawString("< Back", 4, HEADER_HEIGHT / 2 + 1);
    }
}

void uiClearContent() {
    tft.fillRect(0, 0, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_BG);
}

// ============================================================
// Helpers
// ============================================================

void uiDrawStatBar(int x, int y, int w, int h, int value, int maxVal, uint16_t color, const char* label) {
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_WHITE, COLOR_BG);
    tft.setTextSize(1);
    tft.drawString(label, x, y + h / 2);

    int barX = x + 55;
    int barW = w - 55;
    tft.drawRect(barX, y, barW, h, COLOR_ASH);
    int fillW = (int)((float)value / maxVal * (barW - 2));
    if (fillW > 0) {
        tft.fillRect(barX + 1, y + 1, fillW, h - 2, color);
    }

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, x + w, y + h / 2);
}

void uiDrawMenuItem(int y, const char* text, bool selected, uint16_t color) {
    int h = 24;
    if (selected) {
        tft.fillRect(0, y, SCREEN_WIDTH, h, COLOR_DARK_RED);
        tft.setTextColor(TFT_WHITE, COLOR_DARK_RED);
    } else {
        tft.fillRect(0, y, SCREEN_WIDTH, h, COLOR_BG);
        tft.setTextColor(color, COLOR_BG);
    }
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(1);
    tft.drawString(text, 10, y + h / 2);
}

void uiDrawCenteredText(const char* text, int y, uint16_t color, int textSize) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(color, COLOR_BG);
    tft.setTextSize(textSize);
    tft.drawString(text, SCREEN_WIDTH / 2, y);
}

// ============================================================
// Boot Screen
// ============================================================

void uiDrawBootScreen() {
    tft.fillScreen(COLOR_BG);
    uiDrawCenteredText("RAISING HELL", 60, COLOR_HELLFIRE, 3);
    uiDrawCenteredText("CYD Edition", 95, COLOR_BRIMSTONE, 2);

    // Draw a small devil icon
    tft.fillSmoothCircle(160, 150, 20, COLOR_DARK_RED);
    tft.fillTriangle(143, 132, 148, 118, 153, 132, COLOR_HELLFIRE);
    tft.fillTriangle(167, 132, 172, 118, 177, 132, COLOR_HELLFIRE);
    tft.fillSmoothCircle(153, 146, 4, COLOR_HELLFIRE);
    tft.fillSmoothCircle(167, 146, 4, COLOR_HELLFIRE);
    tft.fillSmoothCircle(160, 158, 8, COLOR_HELLFIRE);
    tft.fillRect(152, 150, 16, 8, COLOR_BG);

    uiDrawCenteredText("Touch to start", 200, COLOR_ASH, 1);
}

// ============================================================
// Choose Pet Screen
// ============================================================

void uiDrawChoosePetScreen(int selectedIndex) {
    uiClearContent();
    uiDrawHeader("Choose Your Pet");

    // 2 rows x 3 columns grid
    int colW = SCREEN_WIDTH / 3;
    int rowH = (CONTENT_HEIGHT - HEADER_HEIGHT) / 2;
    int startY = HEADER_HEIGHT;

    for (int i = 0; i < PET_TYPE_COUNT; i++) {
        int col = i % 3;
        int row = i / 3;
        int cx = col * colW + colW / 2;
        int cy = startY + row * rowH + rowH / 2;

        bool selected = (i == selectedIndex);

        if (selected) {
            tft.drawRect(col * colW + 2, startY + row * rowH + 2,
                         colW - 4, rowH - 4, COLOR_HELLFIRE);
            tft.drawRect(col * colW + 3, startY + row * rowH + 3,
                         colW - 6, rowH - 6, COLOR_HELLFIRE);
        }

        // Draw pet preview
        mainSprite.fillSprite(COLOR_BG);
        drawPetPreview(mainSprite, (PetType)i, mainSprite.width() / 2,
                       mainSprite.height() / 2 - 8, 40);
        mainSprite.pushSprite(cx - mainSprite.width() / 2, cy - mainSprite.height() / 2 - 5);

        // Pet type name
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(1);
        tft.setTextColor(selected ? COLOR_SULFUR : TFT_WHITE, COLOR_BG);
        tft.drawString(petTypeName((PetType)i), cx, cy + rowH / 2 - 12);
    }

    uiDrawTabBar(TAB_PET);
}

// ============================================================
// Name Entry Screen (on-screen keyboard)
// ============================================================

static const char KEYBOARD_ROWS[][14] = {
    "ABCDEFGHIJKL",
    "MNOPQRSTUVWX",
    "YZ0123456789",
};
#define KB_ROWS 3
#define KB_COLS 12
#define KB_KEY_W 24
#define KB_KEY_H 28

void uiDrawNameEntryScreen(const char* currentName, int cursorPos, int selectedRow, int selectedCol) {
    uiClearContent();
    uiDrawHeader("Name Your Pet");

    // Show current name with cursor
    char display[PET_NAME_MAX + 2];
    memset(display, 0, sizeof(display));
    strncpy(display, currentName, PET_NAME_MAX);
    if (cursorPos < PET_NAME_MAX) {
        // Show underscore at cursor
        if ((millis() / 500) % 2 == 0) {
            if (cursorPos < (int)strlen(display)) {
                display[cursorPos] = '_';
            } else {
                display[cursorPos] = '_';
                display[cursorPos + 1] = '\0';
            }
        }
    }
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_SULFUR, COLOR_BG);
    tft.setTextSize(2);
    tft.drawString(display, SCREEN_WIDTH / 2, HEADER_HEIGHT + 20);

    // Draw keyboard grid
    int kbStartX = (SCREEN_WIDTH - KB_COLS * KB_KEY_W) / 2;
    int kbStartY = HEADER_HEIGHT + 45;

    for (int r = 0; r < KB_ROWS; r++) {
        for (int c = 0; c < KB_COLS; c++) {
            char ch = KEYBOARD_ROWS[r][c];
            if (ch == '\0') continue;

            int kx = kbStartX + c * KB_KEY_W;
            int ky = kbStartY + r * KB_KEY_H;
            bool sel = (r == selectedRow && c == selectedCol);

            uint16_t bg = sel ? COLOR_HELLFIRE : COLOR_ASH;
            uint16_t fg = sel ? TFT_WHITE : COLOR_BONE;

            tft.fillRect(kx + 1, ky + 1, KB_KEY_W - 2, KB_KEY_H - 2, bg);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(fg, bg);
            tft.setTextSize(1);
            char str[2] = {ch, '\0'};
            tft.drawString(str, kx + KB_KEY_W / 2, ky + KB_KEY_H / 2);
        }
    }

    // Action buttons
    int btnY = kbStartY + KB_ROWS * KB_KEY_H + 5;
    int btnW = 80;

    // DEL button
    bool delSel = (selectedRow == KB_ROWS && selectedCol == 0);
    tft.fillRect(kbStartX, btnY, btnW, KB_KEY_H, delSel ? COLOR_HELLFIRE : COLOR_ASH);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("DEL", kbStartX + btnW / 2, btnY + KB_KEY_H / 2);

    // OK button
    bool okSel = (selectedRow == KB_ROWS && selectedCol == 1);
    tft.fillRect(kbStartX + KB_COLS * KB_KEY_W - btnW, btnY, btnW, KB_KEY_H,
                 okSel ? COLOR_ECTOPLASM : COLOR_ASH);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("OK", kbStartX + KB_COLS * KB_KEY_W - btnW / 2, btnY + KB_KEY_H / 2);

    uiDrawTabBar(TAB_PET);
}

// ============================================================
// Hatching Screen
// ============================================================

void uiDrawHatchingScreen(PetType type, int frame) {
    uiClearContent();

    int cx = SCREEN_WIDTH / 2;
    int cy = CONTENT_HEIGHT / 2;

    if (frame < 30) {
        // Egg shaking
        int shake = (frame % 4 < 2) ? -3 : 3;
        tft.fillSmoothCircle(cx + shake, cy, 30, COLOR_BONE);
        tft.fillSmoothCircle(cx + shake, cy - 10, 25, COLOR_BONE);
        // Cracks
        if (frame > 10) {
            tft.drawLine(cx + shake - 5, cy - 5, cx + shake + 2, cy + 8, COLOR_ASH);
        }
        if (frame > 20) {
            tft.drawLine(cx + shake + 3, cy - 8, cx + shake - 4, cy + 3, COLOR_ASH);
        }
    } else if (frame < 60) {
        // Cracking apart
        float progress = (float)(frame - 30) / 30.0f;
        int spread = (int)(progress * 40);
        tft.fillSmoothCircle(cx - spread, cy, 20 - (int)(progress * 10), COLOR_BONE);
        tft.fillSmoothCircle(cx + spread, cy, 20 - (int)(progress * 10), COLOR_BONE);

        // Pet emerging
        float petScale = progress * 0.5f;
        mainSprite.fillSprite(COLOR_BG);
        drawPet(mainSprite, type, EVO_BABY, MOOD_HAPPY,
                mainSprite.width() / 2, mainSprite.height() / 2, petScale);
        mainSprite.pushSprite(cx - mainSprite.width() / 2, cy - mainSprite.height() / 2);
    } else {
        // Pet fully hatched
        mainSprite.fillSprite(COLOR_BG);
        drawPet(mainSprite, type, EVO_BABY, MOOD_HAPPY,
                mainSprite.width() / 2, mainSprite.height() / 2, 0.7f);
        mainSprite.pushSprite(cx - mainSprite.width() / 2, cy - mainSprite.height() / 2);

        uiDrawCenteredText("It's here!", cy + 60, COLOR_SULFUR, 2);
    }

    uiDrawTabBar(TAB_PET);
}

// ============================================================
// Pet Screen (main view)
// ============================================================

void uiDrawPetScreen() {
    mainSprite.fillSprite(COLOR_BG);

    static float breathScale = 1.0f;
    static float breathDir = 0.003f;
    breathScale += pet.isSleeping ? (breathDir * 0.5f) : breathDir;
    if (breathScale > 1.06f || breathScale < 0.94f) breathDir *= -1;

    int cx = mainSprite.width() / 2;
    int cy = mainSprite.height() / 2 - 15;

    // Draw pet
    drawPet(mainSprite, pet.type, pet.evoStage, pet.getMood(), cx, cy, breathScale);

    // Name and level badge
    mainSprite.setTextDatum(MC_DATUM);
    mainSprite.setTextColor(COLOR_SULFUR, COLOR_BG);
    mainSprite.setTextSize(1);

    char nameBuf[40];
    snprintf(nameBuf, sizeof(nameBuf), "%s  Lv%d", pet.getName(), pet.level);
    mainSprite.drawString(nameBuf, cx, 8);

    // Mood indicator
    mainSprite.setTextColor(petMoodColor(pet.getMood()), COLOR_BG);
    mainSprite.drawString(moodName(pet.getMood()), cx, mainSprite.height() - 18);

    // Sleep overlay
    if (pet.isSleeping) {
        mainSprite.setTextColor(COLOR_SOUL, COLOR_BG);
        mainSprite.setTextSize(2);
        for (int i = 0; i < 3; i++) {
            float zOff = (float)((millis() + i * 800) % 2400) / 2400.0f;
            int zx = cx + 30 + (int)(zOff * 40);
            int zy = cy - 20 - (int)(zOff * 50);
            int zSize = 2 - (int)(zOff * 1.5f);
            if (zSize > 0) {
                mainSprite.setTextSize(zSize);
                mainSprite.drawString("Z", zx, zy);
            }
        }
    }

    // Push sprite to screen (centered in content area)
    int sprX = (SCREEN_WIDTH - mainSprite.width()) / 2;
    int sprY = (CONTENT_HEIGHT - mainSprite.height()) / 2;
    mainSprite.pushSprite(sprX, sprY);
}

// ============================================================
// Stats Screen
// ============================================================

void uiDrawStatsScreen() {
    uiClearContent();
    uiDrawHeader("Stats");

    int y = HEADER_HEIGHT + 8;
    int barH = 14;
    int spacing = 22;
    int margin = 8;
    int barW = SCREEN_WIDTH - margin * 2;

    uiDrawStatBar(margin, y, barW, barH, pet.hunger, 100, COLOR_HUNGER_BAR, "Hunger");
    y += spacing;
    uiDrawStatBar(margin, y, barW, barH, pet.happiness, 100, COLOR_HAPPY_BAR, "Happy");
    y += spacing;
    uiDrawStatBar(margin, y, barW, barH, pet.energy, 100, COLOR_ENERGY_BAR, "Energy");
    y += spacing;
    uiDrawStatBar(margin, y, barW, barH, pet.health, 100, COLOR_HEALTH_BAR, "Health");
    y += spacing + 4;

    // XP bar
    uint32_t xpNeeded = pet.xpForNextLevel();
    uiDrawStatBar(margin, y, barW, barH, pet.xp, xpNeeded, COLOR_INFERNAL, "  XP");
    y += spacing + 4;

    // Info line
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_SULFUR, COLOR_BG);

    char buf[64];
    snprintf(buf, sizeof(buf), "Level: %d   Stage: %s", pet.level, evoStageName((EvoStage)pet.evoStage));
    tft.drawString(buf, margin, y);
    y += 16;

    snprintf(buf, sizeof(buf), "Type: %s   INF: %ld", petTypeName(pet.type), (long)pet.inf);
    tft.drawString(buf, margin, y);
    y += 16;

    char ageBuf[24];
    pet.getAgeString(ageBuf, sizeof(ageBuf));
    snprintf(buf, sizeof(buf), "Age: %s   Mood: %s", ageBuf, moodName(pet.getMood()));
    tft.drawString(buf, margin, y);

    uiDrawTabBar(TAB_STATS);
}

// ============================================================
// Feed Menu
// ============================================================

void uiDrawFeedMenu(int selectedIndex) {
    uiClearContent();
    uiDrawHeader("Feed", true);

    int y = HEADER_HEIGHT + 4;
    int visCount = inventory.getVisibleCount();

    if (visCount == 0) {
        uiDrawCenteredText("No food items!", CONTENT_HEIGHT / 2, COLOR_ASH, 2);
    } else {
        for (int i = 0; i < visCount && i < 7; i++) {
            ItemType type = inventory.getVisibleType(i);
            int qty = inventory.getQuantity(type);
            char buf[48];
            snprintf(buf, sizeof(buf), "%s x%d", itemName(type, pet.type), qty);
            uiDrawMenuItem(y, buf, i == selectedIndex);

            // Show effect preview if selected
            if (i == selectedIndex) {
                ItemDeltas d = itemPreviewDeltas(type);
                tft.setTextDatum(MR_DATUM);
                tft.setTextColor(COLOR_ECTOPLASM, COLOR_DARK_RED);
                char effect[32];
                if (type == ITEM_ELDRITCH_EYE) {
                    snprintf(effect, sizeof(effect), "EVOLVE");
                } else {
                    snprintf(effect, sizeof(effect), "+%dH +%dJ +%dE +%dHP",
                             d.hunger, d.happiness, d.energy, d.health);
                }
                tft.drawString(effect, SCREEN_WIDTH - 6, y + 12);
            }
            y += 24;
        }
    }

    uiDrawTabBar(TAB_FEED);
}

// ============================================================
// Play Menu (mini-game select)
// ============================================================

static const char* miniGameNames[] = {
    "Flappy Fireball",
    "Crossy Hell",
    "Infernal Dodger",
    "Resurrection Run"
};

void uiDrawPlayMenu(int selectedIndex) {
    uiClearContent();
    uiDrawHeader("Play", true);

    int y = HEADER_HEIGHT + 8;

    for (int i = 0; i < 4; i++) {
        bool available = true;
        // Resurrection Run only available when pet is dead
        if (i == 3 && !pet.isDead()) available = false;

        char buf[48];
        if (available) {
            snprintf(buf, sizeof(buf), "%s", miniGameNames[i]);
        } else {
            snprintf(buf, sizeof(buf), "%s (locked)", miniGameNames[i]);
        }

        uint16_t col = available ? TFT_WHITE : COLOR_ASH;
        uiDrawMenuItem(y, buf, i == selectedIndex, col);
        y += 28;
    }

    // Cost info
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_ASH, COLOR_BG);
    tft.setTextSize(1);
    tft.drawString("Costs 5 energy to play", SCREEN_WIDTH / 2, CONTENT_HEIGHT - 20);

    uiDrawTabBar(TAB_PLAY);
}

// ============================================================
// Sleep Menu
// ============================================================

static const char* sleepOptions[] = {
    "Quick Nap (30s)",
    "Full Rest (until 100%)",
    "Light Sleep (until 80%)",
    "Cancel"
};
#define SLEEP_OPTIONS_COUNT 4

void uiDrawSleepMenu(int selectedIndex) {
    uiClearContent();
    uiDrawHeader("Sleep", true);

    // Show current energy
    char buf[32];
    snprintf(buf, sizeof(buf), "Energy: %d/100", pet.energy);
    uiDrawCenteredText(buf, HEADER_HEIGHT + 10, COLOR_ENERGY_BAR, 1);

    int y = HEADER_HEIGHT + 30;
    for (int i = 0; i < SLEEP_OPTIONS_COUNT; i++) {
        uiDrawMenuItem(y, sleepOptions[i], i == selectedIndex);
        y += 28;
    }

    if (pet.isSleeping) {
        uint8_t quality = pet.getSleepQuality();
        snprintf(buf, sizeof(buf), "Sleep Quality: %d%%", quality);
        uiDrawCenteredText(buf, CONTENT_HEIGHT - 20, COLOR_SOUL, 1);
    }

    uiDrawTabBar(TAB_SLEEP);
}

// ============================================================
// Inventory Screen
// ============================================================

void uiDrawInventoryScreen(int selectedIndex) {
    uiClearContent();
    uiDrawHeader("Inventory", true);

    int y = HEADER_HEIGHT + 4;
    int visCount = inventory.getVisibleCount();

    if (visCount == 0) {
        uiDrawCenteredText("Inventory empty", CONTENT_HEIGHT / 2, COLOR_ASH, 2);
    } else {
        for (int i = 0; i < visCount && i < 7; i++) {
            ItemType type = inventory.getVisibleType(i);
            int qty = inventory.getQuantity(type);
            char buf[48];
            snprintf(buf, sizeof(buf), "%s x%d", itemName(type, pet.type), qty);
            uiDrawMenuItem(y, buf, i == selectedIndex);

            if (i == selectedIndex) {
                tft.setTextDatum(MR_DATUM);
                tft.setTextColor(COLOR_ASH, (i == selectedIndex) ? COLOR_DARK_RED : COLOR_BG);
                tft.drawString(itemDesc(type), SCREEN_WIDTH - 6, y + 12);
            }
            y += 24;
        }
    }

    // Show INF balance
    char buf[32];
    snprintf(buf, sizeof(buf), "INF: %ld", (long)pet.inf);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(COLOR_GOLD, COLOR_BG);
    tft.setTextSize(1);
    tft.drawString(buf, SCREEN_WIDTH - 8, CONTENT_HEIGHT - 12);

    uiDrawTabBar(TAB_INV);
}

// ============================================================
// Shop Screen
// ============================================================

void uiDrawShopScreen(int selectedIndex) {
    uiClearContent();
    uiDrawHeader("Shop", true);

    // INF balance at top
    char balBuf[32];
    snprintf(balBuf, sizeof(balBuf), "INF: %ld", (long)pet.inf);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(COLOR_GOLD, COLOR_DARK_RED);
    tft.drawString(balBuf, SCREEN_WIDTH - 8, HEADER_HEIGHT / 2 + 1);

    int y = HEADER_HEIGHT + 4;
    for (int i = 0; i < SHOP_ITEM_COUNT; i++) {
        const ShopItem &item = shopCatalog[i];
        bool canAfford = (pet.inf >= item.price);

        char buf[48];
        snprintf(buf, sizeof(buf), "%s - %d INF",
                 itemName(item.type, pet.type), item.price);

        uint16_t col = canAfford ? TFT_WHITE : COLOR_ASH;
        uiDrawMenuItem(y, buf, i == selectedIndex, col);

        if (i == selectedIndex && !canAfford) {
            tft.setTextDatum(MR_DATUM);
            tft.setTextColor(COLOR_HELLFIRE, COLOR_DARK_RED);
            tft.drawString("Can't afford", SCREEN_WIDTH - 6, y + 12);
        }
        y += 24;
    }

    uiDrawTabBar(TAB_SHOP);
}

// ============================================================
// Death Screen
// ============================================================

void uiDrawDeathScreen(int selectedIndex) {
    uiClearContent();

    uiDrawCenteredText("YOUR PET", 30, COLOR_HELLFIRE, 2);
    uiDrawCenteredText("HAS PERISHED", 55, COLOR_HELLFIRE, 2);

    // Skull icon
    tft.fillSmoothCircle(160, 100, 18, COLOR_BONE);
    tft.fillRect(148, 108, 24, 12, COLOR_BONE);
    tft.fillCircle(154, 97, 4, TFT_BLACK);
    tft.fillCircle(166, 97, 4, TFT_BLACK);
    tft.fillRect(157, 112, 2, 6, TFT_BLACK);
    tft.fillRect(161, 112, 2, 6, TFT_BLACK);
    tft.fillRect(165, 112, 2, 6, TFT_BLACK);

    int y = 130;
    const char* deathOptions[] = { "Resurrection Run", "Bury & Start New" };
    for (int i = 0; i < 2; i++) {
        uiDrawMenuItem(y, deathOptions[i], i == selectedIndex,
                       i == 0 ? COLOR_ECTOPLASM : COLOR_BONE);
        y += 28;
    }

    uiDrawTabBar(TAB_PET);
}

// ============================================================
// Burial Screen
// ============================================================

void uiDrawBurialScreen() {
    tft.fillScreen(COLOR_BG);

    uiDrawCenteredText("Rest in Peace", 50, COLOR_BONE, 2);

    char buf[32];
    snprintf(buf, sizeof(buf), "\"%s\"", pet.getName());
    uiDrawCenteredText(buf, 80, COLOR_SULFUR, 2);

    // Simple headstone
    tft.fillRect(140, 110, 40, 50, COLOR_ASH);
    tft.fillSmoothCircle(160, 115, 20, COLOR_ASH);
    tft.drawLine(155, 120, 165, 120, TFT_BLACK);
    tft.drawLine(160, 115, 160, 130, TFT_BLACK);

    uiDrawCenteredText("Touch to continue...", 190, COLOR_ASH, 1);
}

// ============================================================
// Evolution Screen
// ============================================================

void uiDrawEvolutionScreen(int phase, uint8_t fromStage, uint8_t toStage) {
    tft.fillScreen(COLOR_BG);
    int cx = SCREEN_WIDTH / 2;
    int cy = CONTENT_HEIGHT / 2 - 10;

    if (phase == 0) {
        // Message
        uiDrawCenteredText("Evolution", 30, COLOR_INFERNAL, 2);
        uiDrawCenteredText("has started!", 55, COLOR_INFERNAL, 2);

        // Show old form
        mainSprite.fillSprite(COLOR_BG);
        drawPet(mainSprite, pet.type, fromStage, MOOD_HAPPY,
                mainSprite.width() / 2, mainSprite.height() / 2, 0.8f);
        mainSprite.pushSprite(cx - mainSprite.width() / 2, cy - mainSprite.height() / 2);
    } else if (phase == 1) {
        // Flash white
        tft.fillScreen(TFT_WHITE);
    } else if (phase == 2) {
        // Show new form
        uiDrawCenteredText("Evolution Complete!", 20, COLOR_SULFUR, 2);

        mainSprite.fillSprite(COLOR_BG);
        drawPet(mainSprite, pet.type, toStage, MOOD_HAPPY,
                mainSprite.width() / 2, mainSprite.height() / 2, 0.9f);
        mainSprite.pushSprite(cx - mainSprite.width() / 2, cy - mainSprite.height() / 2);

        char buf[32];
        snprintf(buf, sizeof(buf), "%s -> %s",
                 evoStageName((EvoStage)fromStage), evoStageName((EvoStage)toStage));
        uiDrawCenteredText(buf, CONTENT_HEIGHT - 20, COLOR_BRIMSTONE, 1);
    }
}

// ============================================================
// Level Up Popup
// ============================================================

void uiDrawLevelUpPopup(uint16_t newLevel) {
    // Overlay box
    int bx = 60, by = 60, bw = 200, bh = 80;
    tft.fillRect(bx, by, bw, bh, COLOR_DARK_RED);
    tft.drawRect(bx, by, bw, bh, COLOR_SULFUR);
    tft.drawRect(bx + 1, by + 1, bw - 2, bh - 2, COLOR_SULFUR);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_SULFUR, COLOR_DARK_RED);
    tft.setTextSize(2);
    tft.drawString("LEVEL UP!", bx + bw / 2, by + 25);

    char buf[16];
    snprintf(buf, sizeof(buf), "Level %d", newLevel);
    tft.setTextColor(TFT_WHITE, COLOR_DARK_RED);
    tft.drawString(buf, bx + bw / 2, by + 55);
}

// ============================================================
// Settings Screen
// ============================================================

static const char* settingsLabels[] = {
    "Decay: ",
    "Sound: ",
    "Factory Reset",
    "Back"
};
static const char* decayModeNames[] = {
    "Super Slow", "Slow", "Normal", "Fast", "Very Fast", "Insane"
};
#define SETTINGS_COUNT 4

void uiDrawSettingsScreen(int selectedIndex) {
    uiClearContent();
    uiDrawHeader("Settings");

    int y = HEADER_HEIGHT + 8;
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        char buf[48];
        if (i == 0) {
            DecayMode dm = saveManagerGetDecayMode();
            snprintf(buf, sizeof(buf), "%s%s", settingsLabels[i], decayModeNames[(int)dm]);
        } else if (i == 1) {
            snprintf(buf, sizeof(buf), "%s%s", settingsLabels[i],
                     saveManagerGetSoundEnabled() ? "ON" : "OFF");
        } else {
            snprintf(buf, sizeof(buf), "%s", settingsLabels[i]);
        }

        uint16_t col = (i == 2) ? COLOR_HELLFIRE : TFT_WHITE;
        uiDrawMenuItem(y, buf, i == selectedIndex, col);
        y += 28;
    }

    uiDrawTabBar(TAB_PET);
}
