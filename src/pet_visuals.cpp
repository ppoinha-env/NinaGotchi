#include "pet_visuals.h"
#include <math.h>

// ============================================================
// Color Palette
// ============================================================

uint16_t petPrimaryColor(PetType type) {
    switch (type) {
        case PET_DEVIL:    return COLOR_HELLFIRE;    // Red
        case PET_ELDRITCH: return COLOR_INFERNAL;    // Purple
        case PET_ALIEN:    return COLOR_ECTOPLASM;   // Green
        case PET_KAIJU:    return COLOR_BRIMSTONE;   // Orange
        case PET_ANUBIS:   return COLOR_GOLD;        // Gold
        case PET_AXOLOTL:  return 0xFC18;            // Pink
        default:           return COLOR_BONE;
    }
}

static uint16_t petSecondaryColor(PetType type) {
    switch (type) {
        case PET_DEVIL:    return COLOR_DARK_RED;
        case PET_ELDRITCH: return COLOR_ELDRITCH;
        case PET_ALIEN:    return 0x03E0;           // Dark green
        case PET_KAIJU:    return 0x8400;            // Dark orange
        case PET_ANUBIS:   return 0x8400;            // Dark gold
        case PET_AXOLOTL:  return 0xA00F;            // Dark pink
        default:           return COLOR_ASH;
    }
}

uint16_t petMoodColor(PetMood mood) {
    switch (mood) {
        case MOOD_HAPPY:   return COLOR_SULFUR;      // Yellow
        case MOOD_BORED:   return COLOR_MID_GRAY;
        case MOOD_HUNGRY:  return COLOR_BRIMSTONE;    // Orange
        case MOOD_TIRED:   return COLOR_SOUL;         // Cyan
        case MOOD_MAD:     return COLOR_HELLFIRE;      // Red
        case MOOD_SICK:    return COLOR_ECTOPLASM;     // Green
        default:           return TFT_WHITE;
    }
}

// ============================================================
// Size scaling per evolution stage
// ============================================================

static float evoScale(uint8_t stage) {
    switch (stage) {
        case EVO_BABY:  return 0.55f;
        case EVO_TEEN:  return 0.72f;
        case EVO_ADULT: return 0.90f;
        case EVO_ELDER: return 1.00f;
        default:        return 0.7f;
    }
}

// ============================================================
// Shared drawing helpers
// ============================================================

static void drawEyes(TFT_eSprite &spr, int cx, int cy, float s, PetMood mood,
                     uint16_t col, uint16_t moodCol) {
    int eyeSep = (int)(30 * s);
    int eyeY = cy - (int)(8 * s);
    int eyeR = (int)(8 * s);
    bool blink = ((millis() % 3000) < 180);

    switch (mood) {
        case MOOD_HAPPY:
            // Upward arches (happy eyes)
            if (!blink) {
                spr.fillSmoothCircle(cx - eyeSep, eyeY, eyeR, col);
                spr.fillSmoothCircle(cx + eyeSep, eyeY, eyeR, col);
                // Sparkle
                spr.fillCircle(cx - eyeSep - 2, eyeY - 3, 2, TFT_WHITE);
                spr.fillCircle(cx + eyeSep - 2, eyeY - 3, 2, TFT_WHITE);
            }
            break;
        case MOOD_TIRED:
            // Droopy half-closed lines
            spr.drawFastHLine(cx - eyeSep - eyeR, eyeY, eyeR * 2, col);
            spr.drawFastHLine(cx + eyeSep - eyeR, eyeY, eyeR * 2, col);
            spr.drawFastHLine(cx - eyeSep - eyeR, eyeY + 1, eyeR * 2, col);
            spr.drawFastHLine(cx + eyeSep - eyeR, eyeY + 1, eyeR * 2, col);
            break;
        case MOOD_SICK:
            // Spiral/dizzy eyes
            spr.drawCircle(cx - eyeSep, eyeY, eyeR, moodCol);
            spr.drawCircle(cx - eyeSep, eyeY, eyeR / 2, moodCol);
            spr.drawCircle(cx + eyeSep, eyeY, eyeR, moodCol);
            spr.drawCircle(cx + eyeSep, eyeY, eyeR / 2, moodCol);
            break;
        case MOOD_MAD:
            // Angry slanted eyebrows + eyes
            if (!blink) {
                spr.fillSmoothCircle(cx - eyeSep, eyeY, eyeR, col);
                spr.fillSmoothCircle(cx + eyeSep, eyeY, eyeR, col);
            }
            // Angry brows
            spr.drawLine(cx - eyeSep - eyeR, eyeY - eyeR - 4,
                         cx - eyeSep + eyeR, eyeY - eyeR + 2, moodCol);
            spr.drawLine(cx + eyeSep + eyeR, eyeY - eyeR - 4,
                         cx + eyeSep - eyeR, eyeY - eyeR + 2, moodCol);
            break;
        case MOOD_HUNGRY:
            // Big round pleading eyes
            if (!blink) {
                spr.fillSmoothCircle(cx - eyeSep, eyeY, eyeR + 2, col);
                spr.fillSmoothCircle(cx + eyeSep, eyeY, eyeR + 2, col);
                spr.fillCircle(cx - eyeSep + 1, eyeY - 2, 3, TFT_WHITE);
                spr.fillCircle(cx + eyeSep + 1, eyeY - 2, 3, TFT_WHITE);
            }
            break;
        case MOOD_BORED:
        default:
            // Normal half-lidded
            if (!blink) {
                spr.fillSmoothCircle(cx - eyeSep, eyeY, eyeR, col);
                spr.fillSmoothCircle(cx + eyeSep, eyeY, eyeR, col);
            }
            // Slight droop line
            spr.drawFastHLine(cx - eyeSep - eyeR, eyeY - eyeR, eyeR * 2, TFT_BLACK);
            spr.drawFastHLine(cx + eyeSep - eyeR, eyeY - eyeR, eyeR * 2, TFT_BLACK);
            break;
    }
}

static void drawMouth(TFT_eSprite &spr, int cx, int cy, float s, PetMood mood, uint16_t col) {
    int mouthY = cy + (int)(25 * s);
    int mouthW = (int)(15 * s);

    switch (mood) {
        case MOOD_HAPPY:
            // Big smile
            spr.fillSmoothCircle(cx, mouthY, mouthW, col);
            spr.fillRect(cx - mouthW - 2, mouthY - mouthW, mouthW * 2 + 4, mouthW, TFT_BLACK);
            break;
        case MOOD_TIRED:
            // Small O (yawn)
            spr.drawCircle(cx, mouthY, mouthW / 2, col);
            break;
        case MOOD_SICK:
            // Wavy line
            for (int i = -mouthW; i < mouthW; i++) {
                int dy = (int)(sin(i * 0.5f) * 3);
                spr.drawPixel(cx + i, mouthY + dy, col);
            }
            break;
        case MOOD_MAD:
            // Zigzag frown
            spr.drawLine(cx - mouthW, mouthY, cx - mouthW / 2, mouthY + 4, col);
            spr.drawLine(cx - mouthW / 2, mouthY + 4, cx, mouthY - 2, col);
            spr.drawLine(cx, mouthY - 2, cx + mouthW / 2, mouthY + 4, col);
            spr.drawLine(cx + mouthW / 2, mouthY + 4, cx + mouthW, mouthY, col);
            break;
        case MOOD_HUNGRY:
            // Open drooling mouth
            spr.fillSmoothCircle(cx, mouthY, mouthW - 2, col);
            spr.fillRect(cx - mouthW, mouthY - mouthW, mouthW * 2, mouthW, TFT_BLACK);
            // Drool
            spr.fillRect(cx + mouthW / 2, mouthY, 3, (int)(8 * s), COLOR_SOUL);
            break;
        case MOOD_BORED:
        default:
            // Flat line
            spr.drawFastHLine(cx - mouthW, mouthY, mouthW * 2, col);
            break;
    }
}

// ============================================================
// Devil — Horns, pointed tail, classic demon
// ============================================================

static void drawDevil(TFT_eSprite &spr, int cx, int cy, float s,
                      uint8_t stage, PetMood mood) {
    uint16_t col = petPrimaryColor(PET_DEVIL);
    uint16_t sec = petSecondaryColor(PET_DEVIL);
    uint16_t mCol = petMoodColor(mood);
    int bodyR = (int)(45 * s);

    // Body circle
    spr.fillSmoothCircle(cx, cy, bodyR, sec);
    spr.drawSmoothArc(cx, cy, bodyR, bodyR - 3, 0, 360, col, TFT_BLACK);

    // Horns — bigger with stage
    int hornH = (int)((15 + stage * 5) * s);
    int hornW = (int)(10 * s);
    spr.fillTriangle(cx - bodyR + 10, cy - bodyR + 5,
                     cx - bodyR + 10 - hornW, cy - bodyR - hornH,
                     cx - bodyR + 10 + hornW / 2, cy - bodyR + 5, col);
    spr.fillTriangle(cx + bodyR - 10, cy - bodyR + 5,
                     cx + bodyR - 10 + hornW, cy - bodyR - hornH,
                     cx + bodyR - 10 - hornW / 2, cy - bodyR + 5, col);

    // Tail
    if (stage >= EVO_TEEN) {
        int tailX = cx + bodyR - 5;
        int tailY = cy + bodyR / 2;
        spr.drawLine(tailX, tailY, tailX + 20 * s, tailY + 10 * s, col);
        spr.drawLine(tailX + 20 * s, tailY + 10 * s, tailX + 15 * s, tailY - 5 * s, col);
        // Tail tip
        spr.fillTriangle(tailX + 15 * s - 4, tailY - 5 * s,
                         tailX + 15 * s + 4, tailY - 5 * s,
                         tailX + 15 * s, tailY - 12 * s, col);
    }

    // Elder crown
    if (stage >= EVO_ELDER) {
        for (int i = -2; i <= 2; i++) {
            int flameX = cx + i * (int)(12 * s);
            int flameH = (int)(10 * s + sin(millis() * 0.005 + i) * 4);
            spr.fillTriangle(flameX - 3, cy - bodyR - 5,
                             flameX + 3, cy - bodyR - 5,
                             flameX, cy - bodyR - 5 - flameH, COLOR_SULFUR);
        }
    }

    drawEyes(spr, cx, cy, s, mood, col, mCol);
    drawMouth(spr, cx, cy, s, mood, col);
}

// ============================================================
// Eldritch — Tentacles, single cyclopean eye, cosmic horror
// ============================================================

static void drawEldritch(TFT_eSprite &spr, int cx, int cy, float s,
                         uint8_t stage, PetMood mood) {
    uint16_t col = petPrimaryColor(PET_ELDRITCH);
    uint16_t sec = petSecondaryColor(PET_ELDRITCH);
    uint16_t mCol = petMoodColor(mood);
    int bodyR = (int)(42 * s);

    // Blob body (slightly irregular)
    spr.fillSmoothCircle(cx, cy, bodyR, sec);
    spr.fillSmoothCircle(cx - 5, cy + 3, bodyR - 2, sec);
    spr.drawSmoothArc(cx, cy, bodyR, bodyR - 2, 0, 360, col, TFT_BLACK);

    // Tentacles — more with higher stage
    int numTentacles = 3 + stage;
    for (int i = 0; i < numTentacles; i++) {
        float angle = (float)i / numTentacles * 3.14159f + 1.5f;  // Bottom half
        int tx = cx + (int)(cos(angle) * bodyR * 0.8f);
        int ty = cy + bodyR - 5;
        float wave = sin(millis() * 0.003f + i * 1.5f) * 8 * s;
        spr.drawLine(tx, ty, tx + (int)wave, ty + (int)(20 * s), col);
        spr.drawLine(tx + 1, ty, tx + (int)wave + 1, ty + (int)(20 * s), col);
    }

    // Single large eye (cyclopean)
    int eyeR = (int)(16 * s);
    int eyeY = cy - (int)(5 * s);
    spr.fillSmoothCircle(cx, eyeY, eyeR, TFT_WHITE);

    // Iris — tracks mood
    int irisOffset = 0;
    if (mood == MOOD_BORED) irisOffset = 3;
    if (mood == MOOD_MAD) irisOffset = -2;
    spr.fillSmoothCircle(cx + irisOffset, eyeY, eyeR / 2, col);
    spr.fillCircle(cx + irisOffset, eyeY, eyeR / 4, TFT_BLACK);

    // Sick = bloodshot lines
    if (mood == MOOD_SICK) {
        for (int i = 0; i < 6; i++) {
            float a = i * 1.047f;
            spr.drawLine(cx + cos(a) * eyeR * 0.5f, eyeY + sin(a) * eyeR * 0.5f,
                         cx + cos(a) * eyeR * 0.9f, eyeY + sin(a) * eyeR * 0.9f,
                         COLOR_HELLFIRE);
        }
    }

    // Elder: floating runes
    if (stage >= EVO_ELDER) {
        for (int i = 0; i < 3; i++) {
            float a = millis() * 0.001f + i * 2.094f;
            int rx = cx + (int)(cos(a) * (bodyR + 15));
            int ry = cy + (int)(sin(a) * (bodyR + 15));
            spr.fillCircle(rx, ry, 2, COLOR_INFERNAL);
        }
    }

    drawMouth(spr, cx, cy, s, mood, col);
}

// ============================================================
// Alien — Big head, antenna, green tones
// ============================================================

static void drawAlien(TFT_eSprite &spr, int cx, int cy, float s,
                      uint8_t stage, PetMood mood) {
    uint16_t col = petPrimaryColor(PET_ALIEN);
    uint16_t sec = petSecondaryColor(PET_ALIEN);
    uint16_t mCol = petMoodColor(mood);
    int bodyR = (int)(40 * s);
    int headR = (int)(35 * s);

    // Smaller body below
    spr.fillSmoothCircle(cx, cy + (int)(15 * s), bodyR - 10, sec);

    // Large head
    spr.fillSmoothCircle(cx, cy - (int)(5 * s), headR, sec);
    spr.drawSmoothArc(cx, cy - (int)(5 * s), headR, headR - 2, 0, 360, col, TFT_BLACK);

    // Antenna
    int antH = (int)((12 + stage * 4) * s);
    spr.drawLine(cx, cy - (int)(5 * s) - headR, cx, cy - (int)(5 * s) - headR - antH, col);
    float bobble = sin(millis() * 0.005f) * 3;
    spr.fillSmoothCircle(cx + (int)bobble, cy - (int)(5 * s) - headR - antH, (int)(4 * s), COLOR_ECTOPLASM);

    // Big almond eyes
    int eyeSep = (int)(20 * s);
    int eyeY = cy - (int)(10 * s);
    int eyeW = (int)(14 * s);
    int eyeH = (int)(10 * s);
    bool blink = ((millis() % 4000) < 150);
    if (!blink) {
        spr.fillEllipse(cx - eyeSep, eyeY, eyeW, eyeH, TFT_BLACK);
        spr.fillEllipse(cx - eyeSep, eyeY, eyeW - 2, eyeH - 2, col);
        spr.fillCircle(cx - eyeSep, eyeY, 3, TFT_WHITE);

        spr.fillEllipse(cx + eyeSep, eyeY, eyeW, eyeH, TFT_BLACK);
        spr.fillEllipse(cx + eyeSep, eyeY, eyeW - 2, eyeH - 2, col);
        spr.fillCircle(cx + eyeSep, eyeY, 3, TFT_WHITE);
    }

    // Elder: glow aura
    if (stage >= EVO_ELDER) {
        spr.drawSmoothArc(cx, cy, headR + 10, headR + 8, 0, 360, COLOR_ECTOPLASM, TFT_BLACK);
    }

    drawMouth(spr, cx, cy, s, mood, col);
}

// ============================================================
// Kaiju — Large body, scales, spikes
// ============================================================

static void drawKaiju(TFT_eSprite &spr, int cx, int cy, float s,
                      uint8_t stage, PetMood mood) {
    uint16_t col = petPrimaryColor(PET_KAIJU);
    uint16_t sec = petSecondaryColor(PET_KAIJU);
    uint16_t mCol = petMoodColor(mood);
    int bodyR = (int)(48 * s);

    // Wide body (ellipse-like)
    spr.fillSmoothCircle(cx, cy + 5, bodyR, sec);
    spr.fillSmoothCircle(cx, cy - 5, bodyR - 5, sec);
    spr.drawSmoothArc(cx, cy, bodyR, bodyR - 3, 0, 360, col, TFT_BLACK);

    // Back spikes
    int numSpikes = 3 + stage;
    for (int i = 0; i < numSpikes; i++) {
        float angle = -0.8f + (float)i / numSpikes * 1.6f;
        int sx = cx + (int)(cos(angle - 1.57f) * bodyR);
        int sy = cy + (int)(sin(angle - 1.57f) * bodyR);
        int spikeH = (int)((10 + stage * 3) * s);
        spr.fillTriangle(sx - 4, sy, sx + 4, sy, sx, sy - spikeH, col);
    }

    // Belly plates
    int plateR = (int)(25 * s);
    spr.drawSmoothArc(cx, cy + (int)(8 * s), plateR, plateR - 2, 30, 150, COLOR_SULFUR, sec);

    // Elder: atomic glow
    if (stage >= EVO_ELDER) {
        float pulse = sin(millis() * 0.004f) * 0.3f + 0.7f;
        int glowR = (int)(bodyR * 1.15f * pulse);
        spr.drawSmoothArc(cx, cy, glowR, glowR - 2, 0, 360, COLOR_BRIMSTONE, TFT_BLACK);
    }

    drawEyes(spr, cx, cy, s, mood, col, mCol);
    drawMouth(spr, cx, cy, s, mood, col);
}

// ============================================================
// Anubis — Jackal head, Egyptian motifs
// ============================================================

static void drawAnubis(TFT_eSprite &spr, int cx, int cy, float s,
                       uint8_t stage, PetMood mood) {
    uint16_t col = petPrimaryColor(PET_ANUBIS);
    uint16_t sec = petSecondaryColor(PET_ANUBIS);
    uint16_t mCol = petMoodColor(mood);
    int bodyR = (int)(42 * s);

    // Body (slightly elongated upward)
    spr.fillSmoothCircle(cx, cy + 5, bodyR - 5, sec);
    spr.fillSmoothCircle(cx, cy - 8, bodyR - 8, sec);
    spr.drawSmoothArc(cx, cy, bodyR, bodyR - 2, 0, 360, col, TFT_BLACK);

    // Tall jackal ears
    int earH = (int)((20 + stage * 5) * s);
    int earW = (int)(8 * s);
    spr.fillTriangle(cx - bodyR / 2, cy - bodyR + 5,
                     cx - bodyR / 2 - earW, cy - bodyR - earH,
                     cx - bodyR / 2 + earW, cy - bodyR - earH + 5, col);
    spr.fillTriangle(cx + bodyR / 2, cy - bodyR + 5,
                     cx + bodyR / 2 + earW, cy - bodyR - earH,
                     cx + bodyR / 2 - earW, cy - bodyR - earH + 5, col);

    // Snout (triangular)
    int snoutY = cy + (int)(18 * s);
    spr.fillTriangle(cx - (int)(12 * s), cy + (int)(5 * s),
                     cx + (int)(12 * s), cy + (int)(5 * s),
                     cx, snoutY, sec);
    // Nose
    spr.fillCircle(cx, snoutY - 3, (int)(3 * s), TFT_BLACK);

    // Ankh symbol (for adult+)
    if (stage >= EVO_ADULT) {
        int ax = cx + bodyR + 8;
        int ay = cy - 10;
        spr.drawCircle(ax, ay - 5, 4, col);
        spr.drawFastVLine(ax, ay, 12, col);
        spr.drawFastHLine(ax - 4, ay + 4, 8, col);
    }

    // Elder: eye of Horus glow
    if (stage >= EVO_ELDER) {
        spr.drawSmoothArc(cx, cy, bodyR + 8, bodyR + 6, 0, 360, col, TFT_BLACK);
    }

    drawEyes(spr, cx, cy - (int)(5 * s), s, mood, col, mCol);
}

// ============================================================
// Axolotl — External gills, frilly, pink/cute
// ============================================================

static void drawAxolotl(TFT_eSprite &spr, int cx, int cy, float s,
                        uint8_t stage, PetMood mood) {
    uint16_t col = petPrimaryColor(PET_AXOLOTL);
    uint16_t sec = petSecondaryColor(PET_AXOLOTL);
    uint16_t mCol = petMoodColor(mood);
    int bodyR = (int)(40 * s);

    // Rounded body
    spr.fillSmoothCircle(cx, cy, bodyR, sec);
    spr.drawSmoothArc(cx, cy, bodyR, bodyR - 2, 0, 360, col, TFT_BLACK);

    // External gills (frilly branches on sides of head)
    int numBranches = 3 + stage;
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < numBranches; i++) {
            float angle = -0.7f + (float)i / numBranches * 1.4f;
            int gx = cx + side * (bodyR - 2);
            int gy = cy - (int)(15 * s) + (int)(i * 8 * s);
            float wave = sin(millis() * 0.004f + i + side) * 3;
            int gLen = (int)((15 + stage * 3) * s);
            spr.drawLine(gx, gy, gx + side * gLen + (int)wave, gy - 5, col);
            spr.drawLine(gx + 1, gy, gx + side * gLen + (int)wave + 1, gy - 5, col);
        }
    }

    // Cheek dots (always — cute factor)
    spr.fillSmoothCircle(cx - (int)(25 * s), cy + (int)(8 * s), (int)(5 * s), col);
    spr.fillSmoothCircle(cx + (int)(25 * s), cy + (int)(8 * s), (int)(5 * s), col);

    // Small tail
    if (stage >= EVO_TEEN) {
        int tailY = cy + bodyR - 5;
        float tailWave = sin(millis() * 0.003f) * 10 * s;
        spr.drawLine(cx, tailY, cx + (int)tailWave, tailY + (int)(20 * s), col);
        spr.drawLine(cx + 1, tailY, cx + (int)tailWave + 1, tailY + (int)(20 * s), col);
    }

    // Elder: shimmer particles
    if (stage >= EVO_ELDER) {
        for (int i = 0; i < 5; i++) {
            float a = millis() * 0.002f + i * 1.257f;
            int px = cx + (int)(cos(a) * (bodyR + 12));
            int py = cy + (int)(sin(a) * (bodyR + 12));
            spr.fillCircle(px, py, 2, col);
        }
    }

    drawEyes(spr, cx, cy - (int)(5 * s), s, mood, col, mCol);
    drawMouth(spr, cx, cy, s, mood, col);
}

// ============================================================
// Main dispatch
// ============================================================

void drawPet(TFT_eSprite &spr, PetType type, uint8_t evoStage, PetMood mood,
             int cx, int cy, float breathScale) {
    float s = evoScale(evoStage) * breathScale;

    switch (type) {
        case PET_DEVIL:    drawDevil(spr, cx, cy, s, evoStage, mood);    break;
        case PET_ELDRITCH: drawEldritch(spr, cx, cy, s, evoStage, mood); break;
        case PET_ALIEN:    drawAlien(spr, cx, cy, s, evoStage, mood);    break;
        case PET_KAIJU:    drawKaiju(spr, cx, cy, s, evoStage, mood);    break;
        case PET_ANUBIS:   drawAnubis(spr, cx, cy, s, evoStage, mood);  break;
        case PET_AXOLOTL:  drawAxolotl(spr, cx, cy, s, evoStage, mood); break;
        default:           drawDevil(spr, cx, cy, s, evoStage, mood);    break;
    }
}

void drawPetPreview(TFT_eSprite &spr, PetType type, int cx, int cy, int size) {
    float s = (float)size / 90.0f;  // Normalize to preview size
    drawPet(spr, type, EVO_BABY, MOOD_HAPPY, cx, cy, s);
}
