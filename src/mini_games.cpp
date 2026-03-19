#include "mini_games.h"
#include "ui_renderer.h"
#include "pet.h"
#include "pet_visuals.h"
#include <math.h>

// ============================================================
// Shared Mini-Game State
// ============================================================

static MiniGameType   s_mgType = MG_NONE;
static MiniGameResult s_mgResult = MG_RESULT_PLAYING;
static MiniGameResult s_pendingResult = MG_RESULT_PLAYING;  // Stored until acknowledged
static uint32_t       s_mgStartMs = 0;
static bool           s_mgShowIntro = true;
static uint32_t       s_mgIntroMs = 0;

// Result screen
static bool     s_showResult = false;
static uint32_t s_resultMs = 0;
static bool     s_resultDrawn = false;

// Pause state
static bool     s_mgPaused = false;
static bool     s_pauseWaitRelease = false;

MiniGameResult miniGameGetResult() { return s_mgResult; }

// ============================================================
// Result Screen Helper
// ============================================================

static void drawResultScreen(bool won) {
    tft.fillScreen(COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);

    if (won) {
        tft.setTextColor(COLOR_SULFUR, COLOR_BG);
        tft.drawString("YOU WIN!", SCREEN_WIDTH / 2, 80);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_ECTOPLASM, COLOR_BG);
        tft.drawString("XP and INF earned!", SCREEN_WIDTH / 2, 130);
    } else {
        tft.setTextColor(COLOR_HELLFIRE, COLOR_BG);
        tft.drawString("GAME OVER", SCREEN_WIDTH / 2, 80);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_ASH, COLOR_BG);
        tft.drawString("Better luck next time", SCREEN_WIDTH / 2, 130);
    }

    tft.setTextColor(COLOR_ASH, COLOR_BG);
    tft.drawString("Touch to continue...", SCREEN_WIDTH / 2, 180);
}

// ============================================================
// Intro Screen Helper
// ============================================================

static const char* mgName(MiniGameType t) {
    switch (t) {
        case MG_FLAPPY_FIREBALL:  return "Flappy Fireball";
        case MG_CROSSY_HELL:      return "Crossy Hell";
        case MG_INFERNAL_DODGER:  return "Infernal Dodger";
        case MG_RESURRECTION_RUN: return "Resurrection Run";
        default: return "???";
    }
}

static const char* mgInstructions(MiniGameType t) {
    switch (t) {
        case MG_FLAPPY_FIREBALL:  return "Tap to flap! Avoid pipes.";
        case MG_CROSSY_HELL:      return "Tap edges to move. Reach top!";
        case MG_INFERNAL_DODGER:  return "Tap L/R to steer. Dodge fire!";
        case MG_RESURRECTION_RUN: return "Top=Jump, Bottom=Duck. Run!";
        default: return "";
    }
}

static void drawIntroScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_HELLFIRE, COLOR_BG);
    tft.setTextSize(2);
    tft.drawString(mgName(s_mgType), SCREEN_WIDTH / 2, 60);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, COLOR_BG);
    tft.drawString(mgInstructions(s_mgType), SCREEN_WIDTH / 2, 110);

    tft.setTextColor(COLOR_SULFUR, COLOR_BG);
    tft.drawString("Tap to start!", SCREEN_WIDTH / 2, 160);
}

// ============================================================
//  FLAPPY FIREBALL
// ============================================================

static struct {
    float birdY, birdVel;
    float pipeX[4];
    int   pipeGapY[4];
    int   pipeCount;
    int   score;
    int   nextPipe;
    float scrollSpeed;
    bool  started;
} flappy;

#define FLAPPY_GRAVITY    0.35f
#define FLAPPY_FLAP      -5.5f
#define FLAPPY_BIRD_X     60
#define FLAPPY_BIRD_R     8
#define FLAPPY_PIPE_W     28
#define FLAPPY_GAP_H      65
#define FLAPPY_PIPE_SPEED 2.0f
#define FLAPPY_WIN_SCORE  10

static void flappyInit() {
    flappy.birdY = CONTENT_HEIGHT / 2;
    flappy.birdVel = 0;
    flappy.score = 0;
    flappy.pipeCount = 3;
    flappy.scrollSpeed = FLAPPY_PIPE_SPEED;
    flappy.started = false;
    flappy.nextPipe = 0;
    for (int i = 0; i < flappy.pipeCount; i++) {
        flappy.pipeX[i] = SCREEN_WIDTH + i * 120;
        flappy.pipeGapY[i] = 40 + random(0, CONTENT_HEIGHT - FLAPPY_GAP_H - 40);
    }
}

static void flappyTick(const InputState &input, bool &won, bool &lost) {
    won = false;
    lost = false;

    // Input: tap anywhere to flap
    if (input.anyTouch && input.freshPress && input.touchY < TAB_BAR_Y) {
        flappy.birdVel = FLAPPY_FLAP;
        if (!flappy.started) flappy.started = true;
    }

    if (!flappy.started) {
        // Draw static
        tft.fillRect(0, 0, SCREEN_WIDTH, CONTENT_HEIGHT, 0x0010);  // Dark blue sky
        tft.fillSmoothCircle(FLAPPY_BIRD_X, (int)flappy.birdY, FLAPPY_BIRD_R, COLOR_BRIMSTONE);
        tft.fillSmoothCircle(FLAPPY_BIRD_X + 4, (int)flappy.birdY - 2, 3, COLOR_SULFUR);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, 0x0010);
        tft.setTextSize(1);
        tft.drawString("Tap to flap!", SCREEN_WIDTH / 2, 30);
        return;
    }

    // Physics
    flappy.birdVel += FLAPPY_GRAVITY;
    flappy.birdY += flappy.birdVel;

    // Ceiling / floor
    if (flappy.birdY < FLAPPY_BIRD_R) {
        flappy.birdY = FLAPPY_BIRD_R;
        flappy.birdVel = 0;
    }
    if (flappy.birdY > CONTENT_HEIGHT - FLAPPY_BIRD_R) {
        lost = true;
        return;
    }

    // Move pipes
    for (int i = 0; i < flappy.pipeCount; i++) {
        flappy.pipeX[i] -= flappy.scrollSpeed;

        // Score when pipe passes bird
        if (flappy.pipeX[i] + FLAPPY_PIPE_W < FLAPPY_BIRD_X &&
            flappy.pipeX[i] + FLAPPY_PIPE_W >= FLAPPY_BIRD_X - flappy.scrollSpeed) {
            flappy.score++;
        }

        // Recycle pipe
        if (flappy.pipeX[i] < -FLAPPY_PIPE_W) {
            flappy.pipeX[i] = SCREEN_WIDTH + 20;
            flappy.pipeGapY[i] = 30 + random(0, CONTENT_HEIGHT - FLAPPY_GAP_H - 30);
        }

        // Collision check
        if (FLAPPY_BIRD_X + FLAPPY_BIRD_R > flappy.pipeX[i] &&
            FLAPPY_BIRD_X - FLAPPY_BIRD_R < flappy.pipeX[i] + FLAPPY_PIPE_W) {
            int gapTop = flappy.pipeGapY[i];
            int gapBot = gapTop + FLAPPY_GAP_H;
            if (flappy.birdY - FLAPPY_BIRD_R < gapTop ||
                flappy.birdY + FLAPPY_BIRD_R > gapBot) {
                lost = true;
                return;
            }
        }
    }

    // Win check
    if (flappy.score >= FLAPPY_WIN_SCORE) {
        won = true;
        return;
    }

    // --- Draw ---
    tft.fillRect(0, 0, SCREEN_WIDTH, CONTENT_HEIGHT, 0x0010);  // Dark sky

    // Pipes
    for (int i = 0; i < flappy.pipeCount; i++) {
        int px = (int)flappy.pipeX[i];
        if (px > SCREEN_WIDTH || px + FLAPPY_PIPE_W < 0) continue;
        int gapTop = flappy.pipeGapY[i];
        int gapBot = gapTop + FLAPPY_GAP_H;
        // Top pipe
        tft.fillRect(px, 0, FLAPPY_PIPE_W, gapTop, COLOR_ECTOPLASM);
        tft.fillRect(px - 3, gapTop - 8, FLAPPY_PIPE_W + 6, 8, COLOR_ECTOPLASM);
        // Bottom pipe
        tft.fillRect(px, gapBot, FLAPPY_PIPE_W, CONTENT_HEIGHT - gapBot, COLOR_ECTOPLASM);
        tft.fillRect(px - 3, gapBot, FLAPPY_PIPE_W + 6, 8, COLOR_ECTOPLASM);
    }

    // Bird (fireball)
    tft.fillSmoothCircle(FLAPPY_BIRD_X, (int)flappy.birdY, FLAPPY_BIRD_R, COLOR_BRIMSTONE);
    tft.fillSmoothCircle(FLAPPY_BIRD_X + 3, (int)flappy.birdY - 2, 3, COLOR_SULFUR);
    // Flame trail
    for (int i = 1; i <= 3; i++) {
        int fx = FLAPPY_BIRD_X - i * 6;
        int fy = (int)flappy.birdY + random(-2, 3);
        tft.fillCircle(fx, fy, FLAPPY_BIRD_R - i * 2, COLOR_HELLFIRE);
    }

    // Score
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_WHITE, 0x0010);
    tft.setTextSize(2);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", flappy.score);
    tft.drawString(buf, SCREEN_WIDTH - 10, 10);
}

// ============================================================
//  CROSSY HELL
// ============================================================

static struct {
    int playerCol, playerRow;
    float platformX[5];      // 5 water rows
    float platformSpeed[5];
    int   platformWidth;
    int   goalCol;
    int   score;
    bool  onPlatform;
} crossy;

#define CROSSY_COLS    11
#define CROSSY_ROWS    7
#define CROSSY_TILE_W  (SCREEN_WIDTH / CROSSY_COLS)
#define CROSSY_TILE_H  (CONTENT_HEIGHT / CROSSY_ROWS)
// Row types: 0=start(safe), 1-5=water, 6=goal(safe)

static void crossyInit() {
    crossy.playerCol = CROSSY_COLS / 2;
    crossy.playerRow = 0;
    crossy.platformWidth = 3;
    crossy.goalCol = CROSSY_COLS / 2;
    crossy.score = 0;
    crossy.onPlatform = false;
    for (int i = 0; i < 5; i++) {
        crossy.platformX[i] = random(0, CROSSY_COLS - crossy.platformWidth);
        crossy.platformSpeed[i] = (i % 2 == 0) ? 0.02f : -0.02f;
        crossy.platformSpeed[i] *= (1.0f + i * 0.3f);
    }
}

static void crossyTick(const InputState &input, bool &won, bool &lost) {
    won = false;
    lost = false;

    // Input: tap edges to move
    if (input.freshPress && input.touchY < TAB_BAR_Y) {
        if (input.up && crossy.playerRow < CROSSY_ROWS - 1) {
            crossy.playerRow++;
        } else if (input.down && crossy.playerRow > 0) {
            crossy.playerRow--;
        } else if (input.left && crossy.playerCol > 0) {
            crossy.playerCol--;
        } else if (input.right && crossy.playerCol < CROSSY_COLS - 1) {
            crossy.playerCol++;
        }
    }

    // Move platforms
    for (int i = 0; i < 5; i++) {
        crossy.platformX[i] += crossy.platformSpeed[i];
        if (crossy.platformX[i] > CROSSY_COLS) crossy.platformX[i] = -crossy.platformWidth;
        if (crossy.platformX[i] < -crossy.platformWidth) crossy.platformX[i] = CROSSY_COLS;
    }

    // Check if player is on a water row (rows 1-5)
    if (crossy.playerRow >= 1 && crossy.playerRow <= 5) {
        int waterIdx = crossy.playerRow - 1;
        int platStart = (int)crossy.platformX[waterIdx];
        int platEnd = platStart + crossy.platformWidth;
        if (crossy.playerCol >= platStart && crossy.playerCol < platEnd) {
            crossy.onPlatform = true;
            // Platform carries player
            crossy.playerCol += (crossy.platformSpeed[waterIdx] > 0) ? 0 : 0;
        } else {
            // In water — dead!
            lost = true;
            return;
        }
    }

    // Check out of bounds
    if (crossy.playerCol < 0 || crossy.playerCol >= CROSSY_COLS) {
        lost = true;
        return;
    }

    // Win check — reached top row
    if (crossy.playerRow >= CROSSY_ROWS - 1) {
        won = true;
        return;
    }

    // --- Draw ---
    tft.fillRect(0, 0, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_BG);

    for (int r = 0; r < CROSSY_ROWS; r++) {
        int drawY = CONTENT_HEIGHT - (r + 1) * CROSSY_TILE_H;

        if (r == 0) {
            // Start row — safe ground (dark red)
            tft.fillRect(0, drawY, SCREEN_WIDTH, CROSSY_TILE_H, COLOR_DARK_RED);
        } else if (r == CROSSY_ROWS - 1) {
            // Goal row — with torches
            tft.fillRect(0, drawY, SCREEN_WIDTH, CROSSY_TILE_H, COLOR_DARK_RED);
            // Torches
            int tcx = crossy.goalCol * CROSSY_TILE_W + CROSSY_TILE_W / 2;
            tft.fillRect(tcx - 15, drawY + 2, 4, CROSSY_TILE_H - 4, COLOR_BRIMSTONE);
            tft.fillRect(tcx + 12, drawY + 2, 4, CROSSY_TILE_H - 4, COLOR_BRIMSTONE);
            int flicker = random(-2, 3);
            tft.fillCircle(tcx - 13, drawY + flicker, 4, COLOR_SULFUR);
            tft.fillCircle(tcx + 14, drawY + flicker, 4, COLOR_SULFUR);
        } else {
            // Water/lava row
            uint16_t waterCol = 0x0008;  // Very dark blue
            tft.fillRect(0, drawY, SCREEN_WIDTH, CROSSY_TILE_H, waterCol);

            // Lava ripples
            for (int x = 0; x < SCREEN_WIDTH; x += 20) {
                int ry = drawY + CROSSY_TILE_H / 2 + (int)(sin((x + millis() * 0.002f) * 0.1f) * 3);
                tft.drawPixel(x, ry, COLOR_LAVA);
            }

            // Draw platform
            int waterIdx = r - 1;
            int px = (int)(crossy.platformX[waterIdx] * CROSSY_TILE_W);
            int pw = crossy.platformWidth * CROSSY_TILE_W;
            tft.fillRect(px, drawY + 2, pw, CROSSY_TILE_H - 4, COLOR_ASH);
            tft.drawRect(px, drawY + 2, pw, CROSSY_TILE_H - 4, COLOR_BONE);
        }
    }

    // Draw player
    int playerDrawX = crossy.playerCol * CROSSY_TILE_W + CROSSY_TILE_W / 2;
    int playerDrawY = CONTENT_HEIGHT - (crossy.playerRow + 1) * CROSSY_TILE_H + CROSSY_TILE_H / 2;
    uint16_t playerCol = petPrimaryColor(pet.type);
    tft.fillSmoothCircle(playerDrawX, playerDrawY, CROSSY_TILE_W / 3, playerCol);
    tft.fillCircle(playerDrawX - 3, playerDrawY - 3, 2, TFT_WHITE);
    tft.fillCircle(playerDrawX + 3, playerDrawY - 3, 2, TFT_WHITE);
}

// ============================================================
//  INFERNAL DODGER
// ============================================================

static struct {
    float carX;
    float fireballs[12][2];  // [x, y]
    int   fireballCount;
    uint32_t startMs;
    bool  goalVisible;
    float goalX;
    int   score;
} dodger;

#define DODGER_CAR_W      20
#define DODGER_CAR_H      30
#define DODGER_CAR_Y      (CONTENT_HEIGHT - 45)
#define DODGER_ROAD_LEFT  40
#define DODGER_ROAD_RIGHT (SCREEN_WIDTH - 40)
#define DODGER_ROAD_W     (DODGER_ROAD_RIGHT - DODGER_ROAD_LEFT)
#define DODGER_SURVIVE_MS 12000
#define DODGER_FB_R       6

static void dodgerInit() {
    dodger.carX = SCREEN_WIDTH / 2;
    dodger.fireballCount = 0;
    dodger.startMs = millis();
    dodger.goalVisible = false;
    dodger.goalX = SCREEN_WIDTH / 2;
    dodger.score = 0;

    // Init fireballs
    for (int i = 0; i < 12; i++) {
        dodger.fireballs[i][0] = DODGER_ROAD_LEFT + random(0, DODGER_ROAD_W);
        dodger.fireballs[i][1] = -random(10, 200);
    }
    dodger.fireballCount = 6;
}

static void dodgerTick(const InputState &input, bool &won, bool &lost) {
    won = false;
    lost = false;

    uint32_t elapsed = millis() - dodger.startMs;

    // Input: tap left/right half to steer
    if (input.anyTouch && input.touchY < TAB_BAR_Y) {
        if (input.touchX < SCREEN_WIDTH / 2) {
            dodger.carX -= 4;
        } else {
            dodger.carX += 4;
        }
    }

    // Clamp car to road
    if (dodger.carX < DODGER_ROAD_LEFT + DODGER_CAR_W / 2) {
        // Off-road crash!
        lost = true;
        return;
    }
    if (dodger.carX > DODGER_ROAD_RIGHT - DODGER_CAR_W / 2) {
        lost = true;
        return;
    }

    // After survive time, show goal
    if (elapsed > DODGER_SURVIVE_MS && !dodger.goalVisible) {
        dodger.goalVisible = true;
        dodger.goalX = (DODGER_ROAD_LEFT + DODGER_ROAD_RIGHT) / 2;
    }

    // Move fireballs
    int speedMult = 1 + elapsed / 3000;  // Gets faster over time
    for (int i = 0; i < dodger.fireballCount; i++) {
        dodger.fireballs[i][1] += 2.5f * speedMult;
        if (dodger.fireballs[i][1] > CONTENT_HEIGHT + 20) {
            // Recycle
            dodger.fireballs[i][0] = DODGER_ROAD_LEFT + random(0, DODGER_ROAD_W);
            dodger.fireballs[i][1] = -random(10, 60);
            dodger.score++;
        }

        // Collision with car
        float dx = dodger.fireballs[i][0] - dodger.carX;
        float dy = dodger.fireballs[i][1] - DODGER_CAR_Y;
        if (dx * dx + dy * dy < (DODGER_FB_R + DODGER_CAR_W / 2) * (DODGER_FB_R + DODGER_CAR_W / 2)) {
            lost = true;
            return;
        }
    }

    // Increase fireball count over time
    if (elapsed > 3000 && dodger.fireballCount < 8) dodger.fireballCount = 8;
    if (elapsed > 6000 && dodger.fireballCount < 10) dodger.fireballCount = 10;
    if (elapsed > 9000 && dodger.fireballCount < 12) dodger.fireballCount = 12;

    // Goal collision
    if (dodger.goalVisible) {
        float dx = dodger.goalX - dodger.carX;
        float dy = 60 - DODGER_CAR_Y;
        if (abs(dx) < 25 && DODGER_CAR_Y < 80) {
            won = true;
            return;
        }
        // Auto-drive to goal after it appears
        if (dodger.goalVisible && !input.anyTouch) {
            // Let player drive to it
        }
    }

    // --- Draw ---
    tft.fillRect(0, 0, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_ELDRITCH);  // Off-road

    // Road
    tft.fillRect(DODGER_ROAD_LEFT, 0, DODGER_ROAD_W, CONTENT_HEIGHT, COLOR_ASH);
    // Road lines
    for (int y = (int)(millis() / 50) % 30; y < CONTENT_HEIGHT; y += 30) {
        tft.fillRect(SCREEN_WIDTH / 2 - 2, y, 4, 15, COLOR_SULFUR);
    }
    // Road edges
    tft.fillRect(DODGER_ROAD_LEFT, 0, 3, CONTENT_HEIGHT, TFT_WHITE);
    tft.fillRect(DODGER_ROAD_RIGHT - 3, 0, 3, CONTENT_HEIGHT, TFT_WHITE);

    // Goal
    if (dodger.goalVisible) {
        tft.fillRect((int)dodger.goalX - 15, 40, 30, 30, COLOR_SULFUR);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_BLACK, COLOR_SULFUR);
        tft.setTextSize(1);
        tft.drawString("GOAL", (int)dodger.goalX, 55);
    }

    // Fireballs
    for (int i = 0; i < dodger.fireballCount; i++) {
        int fx = (int)dodger.fireballs[i][0];
        int fy = (int)dodger.fireballs[i][1];
        if (fy < -DODGER_FB_R || fy > CONTENT_HEIGHT) continue;
        tft.fillSmoothCircle(fx, fy, DODGER_FB_R, COLOR_HELLFIRE);
        tft.fillCircle(fx, fy - 2, 3, COLOR_SULFUR);
    }

    // Car
    int cx = (int)dodger.carX;
    tft.fillRect(cx - DODGER_CAR_W / 2, DODGER_CAR_Y - DODGER_CAR_H / 2,
                 DODGER_CAR_W, DODGER_CAR_H, petPrimaryColor(pet.type));
    tft.fillRect(cx - DODGER_CAR_W / 2 + 2, DODGER_CAR_Y - DODGER_CAR_H / 2 + 2,
                 DODGER_CAR_W - 4, 8, TFT_BLACK);  // Windshield
    // Wheels
    tft.fillRect(cx - DODGER_CAR_W / 2 - 3, DODGER_CAR_Y - 8, 4, 8, TFT_BLACK);
    tft.fillRect(cx + DODGER_CAR_W / 2, DODGER_CAR_Y - 8, 4, 8, TFT_BLACK);
    tft.fillRect(cx - DODGER_CAR_W / 2 - 3, DODGER_CAR_Y + 5, 4, 8, TFT_BLACK);
    tft.fillRect(cx + DODGER_CAR_W / 2, DODGER_CAR_Y + 5, 4, 8, TFT_BLACK);

    // Timer
    int remaining = (DODGER_SURVIVE_MS - (int)elapsed) / 1000;
    if (remaining < 0) remaining = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "%ds", remaining);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, COLOR_ASH);
    tft.setTextSize(2);
    tft.drawString(buf, DODGER_ROAD_LEFT + 5, 5);
}

// ============================================================
//  RESURRECTION RUN
// ============================================================

static struct {
    float playerY;       // 0=ground, 1=jump, -1=duck
    float jumpVel;
    bool  isJumping;
    bool  isDucking;
    float scrollX;
    float scrollSpeed;

    // Obstacles: [x, type] — type: 0=ground spike, 1=low fire
    float obstacles[8][2];
    int   obstacleCount;
    float goalX;
    bool  goalReached;
    int   score;
} runner;

#define RUNNER_GROUND_Y   (CONTENT_HEIGHT - 40)
#define RUNNER_PLAYER_X   60
#define RUNNER_PLAYER_W   16
#define RUNNER_PLAYER_H   24
#define RUNNER_JUMP_VEL   -7.0f
#define RUNNER_GRAVITY    0.4f
#define RUNNER_DUCK_H     12
#define RUNNER_OBS_W      16
#define RUNNER_OBS_H      20
#define RUNNER_GOAL_DIST  2400.0f

static void runnerInit() {
    runner.playerY = 0;
    runner.jumpVel = 0;
    runner.isJumping = false;
    runner.isDucking = false;
    runner.scrollX = 0;
    runner.scrollSpeed = 3.0f;
    runner.obstacleCount = 6;
    runner.goalX = RUNNER_GOAL_DIST;
    runner.goalReached = false;
    runner.score = 0;

    for (int i = 0; i < runner.obstacleCount; i++) {
        runner.obstacles[i][0] = 200 + i * 150 + random(0, 80);
        runner.obstacles[i][1] = random(0, 2);  // 0=spike, 1=low fire
    }
}

static void runnerTick(const InputState &input, bool &won, bool &lost) {
    won = false;
    lost = false;

    // Input: top half = jump, bottom half = duck
    if (input.anyTouch && input.touchY < TAB_BAR_Y) {
        if (input.touchY < CONTENT_HEIGHT / 2) {
            // Jump
            if (!runner.isJumping) {
                runner.isJumping = true;
                runner.jumpVel = RUNNER_JUMP_VEL;
                runner.isDucking = false;
            }
        } else {
            // Duck
            if (!runner.isJumping) {
                runner.isDucking = true;
            }
        }
    } else {
        runner.isDucking = false;
    }

    // Jump physics
    if (runner.isJumping) {
        runner.jumpVel += RUNNER_GRAVITY;
        runner.playerY += runner.jumpVel;
        if (runner.playerY >= 0) {
            runner.playerY = 0;
            runner.isJumping = false;
            runner.jumpVel = 0;
        }
    }

    // Scroll
    runner.scrollX += runner.scrollSpeed;
    runner.scrollSpeed += 0.001f;  // Speed up over time

    // Move obstacles relative to scroll
    int playerH = runner.isDucking ? RUNNER_DUCK_H : RUNNER_PLAYER_H;
    int playerDrawY = RUNNER_GROUND_Y + (int)runner.playerY - playerH;

    for (int i = 0; i < runner.obstacleCount; i++) {
        float obsScreenX = runner.obstacles[i][0] - runner.scrollX + SCREEN_WIDTH;
        int obsType = (int)runner.obstacles[i][1];

        // Collision
        if (obsScreenX > RUNNER_PLAYER_X - RUNNER_OBS_W &&
            obsScreenX < RUNNER_PLAYER_X + RUNNER_PLAYER_W) {
            if (obsType == 0) {
                // Ground spike — jump to avoid
                if (!runner.isJumping || runner.playerY > -15) {
                    // Hit if not jumping high enough
                    if (runner.playerY > -RUNNER_OBS_H + 5) {
                        lost = true;
                        return;
                    }
                }
            } else {
                // Low fire — duck to avoid
                if (!runner.isDucking) {
                    lost = true;
                    return;
                }
            }
        }

        // Recycle obstacles that have scrolled past
        if (obsScreenX < -RUNNER_OBS_W) {
            runner.obstacles[i][0] += runner.obstacleCount * 150 + random(0, 100);
            runner.obstacles[i][1] = random(0, 2);
            runner.score++;
        }
    }

    // Goal check
    float goalScreenX = runner.goalX - runner.scrollX + SCREEN_WIDTH;
    if (goalScreenX < RUNNER_PLAYER_X + RUNNER_PLAYER_W) {
        won = true;
        return;
    }

    // --- Draw ---
    tft.fillRect(0, 0, SCREEN_WIDTH, CONTENT_HEIGHT, 0x1000);  // Dark red sky

    // Ground
    tft.fillRect(0, RUNNER_GROUND_Y, SCREEN_WIDTH, CONTENT_HEIGHT - RUNNER_GROUND_Y, COLOR_ASH);
    // Ground line
    tft.drawFastHLine(0, RUNNER_GROUND_Y, SCREEN_WIDTH, COLOR_BONE);

    // Obstacles
    for (int i = 0; i < runner.obstacleCount; i++) {
        float obsScreenX = runner.obstacles[i][0] - runner.scrollX + SCREEN_WIDTH;
        if (obsScreenX < -20 || obsScreenX > SCREEN_WIDTH + 20) continue;
        int ox = (int)obsScreenX;
        int obsType = (int)runner.obstacles[i][1];

        if (obsType == 0) {
            // Ground spike
            tft.fillTriangle(ox, RUNNER_GROUND_Y,
                             ox + RUNNER_OBS_W, RUNNER_GROUND_Y,
                             ox + RUNNER_OBS_W / 2, RUNNER_GROUND_Y - RUNNER_OBS_H,
                             COLOR_HELLFIRE);
        } else {
            // Low fire (overhead)
            int fireY = RUNNER_GROUND_Y - RUNNER_PLAYER_H - 5;
            tft.fillSmoothCircle(ox + RUNNER_OBS_W / 2, fireY, 8, COLOR_BRIMSTONE);
            tft.fillCircle(ox + RUNNER_OBS_W / 2, fireY - 4, 4, COLOR_SULFUR);
            tft.fillRect(ox, fireY + 5, RUNNER_OBS_W, 3, COLOR_ASH);
        }
    }

    // Goal
    if (goalScreenX < SCREEN_WIDTH + 50 && goalScreenX > -50) {
        int gx = (int)goalScreenX;
        // Hand reaching up from ground
        tft.fillRect(gx, RUNNER_GROUND_Y - 35, 8, 35, COLOR_BONE);
        tft.fillCircle(gx + 4, RUNNER_GROUND_Y - 38, 6, COLOR_BONE);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COLOR_SULFUR, 0x1000);
        tft.setTextSize(1);
        tft.drawString("GOAL", gx + 4, RUNNER_GROUND_Y - 50);
    }

    // Player
    int pDrawY = RUNNER_GROUND_Y + (int)runner.playerY;
    uint16_t pCol = petPrimaryColor(pet.type);
    if (runner.isDucking) {
        // Ducking — shorter, wider
        tft.fillRect(RUNNER_PLAYER_X - 2, pDrawY - RUNNER_DUCK_H,
                     RUNNER_PLAYER_W + 4, RUNNER_DUCK_H, pCol);
    } else {
        // Standing/jumping
        tft.fillRect(RUNNER_PLAYER_X, pDrawY - RUNNER_PLAYER_H,
                     RUNNER_PLAYER_W, RUNNER_PLAYER_H, pCol);
        // Eyes
        tft.fillCircle(RUNNER_PLAYER_X + 5, pDrawY - RUNNER_PLAYER_H + 6, 2, TFT_WHITE);
        tft.fillCircle(RUNNER_PLAYER_X + 11, pDrawY - RUNNER_PLAYER_H + 6, 2, TFT_WHITE);
    }

    // Progress bar
    float progress = runner.scrollX / RUNNER_GOAL_DIST;
    if (progress > 1.0f) progress = 1.0f;
    int barW = SCREEN_WIDTH - 20;
    tft.drawRect(10, 5, barW, 8, COLOR_ASH);
    tft.fillRect(11, 6, (int)(progress * (barW - 2)), 6, COLOR_ECTOPLASM);
}

// ============================================================
// Main Dispatcher
// ============================================================

void miniGameStart(MiniGameType type) {
    s_mgType = type;
    s_mgResult = MG_RESULT_PLAYING;
    s_pendingResult = MG_RESULT_PLAYING;
    s_mgShowIntro = true;
    s_mgIntroMs = millis();
    s_showResult = false;
    s_resultDrawn = false;
    s_mgPaused = false;
    s_pauseWaitRelease = false;

    switch (type) {
        case MG_FLAPPY_FIREBALL:  flappyInit(); break;
        case MG_CROSSY_HELL:      crossyInit(); break;
        case MG_INFERNAL_DODGER:  dodgerInit(); break;
        case MG_RESURRECTION_RUN: runnerInit(); break;
        default: break;
    }
}

void miniGameTick(const InputState &input) {
    // Show intro screen
    if (s_mgShowIntro) {
        drawIntroScreen();
        if (input.anyTouch && input.freshPress && (millis() - s_mgIntroMs > 500)) {
            s_mgShowIntro = false;
            s_mgStartMs = millis();
            tft.fillScreen(COLOR_BG);
        }
        return;
    }

    // Show result screen — wait for user acknowledgement
    if (s_showResult) {
        if (!s_resultDrawn) {
            drawResultScreen(s_pendingResult == MG_RESULT_WIN);
            s_resultDrawn = true;
        }
        // Wait at least 1s, then accept touch to dismiss
        if (input.anyTouch && input.freshPress && (millis() - s_resultMs > 1000)) {
            s_mgResult = s_pendingResult;  // Commit result — game_state.cpp will pick it up
        }
        return;
    }

    // Pause state — non-blocking
    if (s_mgPaused) {
        // Draw pause overlay (only once per pause, but redraw is cheap)
        tft.fillRect(80, 70, 160, 60, COLOR_DARK_RED);
        tft.drawRect(80, 70, 160, 60, COLOR_SULFUR);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, COLOR_DARK_RED);
        tft.setTextSize(2);
        tft.drawString("PAUSED", 160, 90);
        tft.setTextSize(1);
        tft.drawString("Tap to resume", 160, 115);

        if (s_pauseWaitRelease) {
            // Wait for finger to be lifted first
            if (!input.anyTouch) {
                s_pauseWaitRelease = false;
            }
        } else {
            // Then wait for new tap to unpause
            if (input.anyTouch && input.freshPress) {
                s_mgPaused = false;
            }
        }
        return;
    }

    // Run active game
    bool won = false, lost = false;

    switch (s_mgType) {
        case MG_FLAPPY_FIREBALL:  flappyTick(input, won, lost); break;
        case MG_CROSSY_HELL:      crossyTick(input, won, lost); break;
        case MG_INFERNAL_DODGER:  dodgerTick(input, won, lost); break;
        case MG_RESURRECTION_RUN: runnerTick(input, won, lost); break;
        default: break;
    }

    // Pause button (top-right corner)
    tft.fillRect(SCREEN_WIDTH - 30, 0, 30, 20, COLOR_ASH);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, COLOR_ASH);
    tft.setTextSize(1);
    tft.drawString("||", SCREEN_WIDTH - 15, 10);

    // Check for pause tap (non-blocking)
    if (input.freshPress && input.touchX > SCREEN_WIDTH - 30 && input.touchY < 20) {
        s_mgPaused = true;
        s_pauseWaitRelease = true;
        return;
    }

    if (won || lost) {
        s_showResult = true;
        s_resultDrawn = false;
        s_resultMs = millis();
        s_pendingResult = won ? MG_RESULT_WIN : MG_RESULT_LOSE;
        // s_mgResult stays MG_RESULT_PLAYING until user acknowledges result screen
    }
}
