#include "touch_input.h"

// --- Raw touch calibration for CYD (rotation=1) ---
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3700
#define TOUCH_Y_MIN 240
#define TOUCH_Y_MAX 3800

// --- Touch zones for menu navigation ---
// Content area: 0..CONTENT_HEIGHT (204px)
// Tab bar: TAB_BAR_Y..SCREEN_HEIGHT (204..240)

// For directional input in content area:
//   Top 25% = UP, Bottom 25% = DOWN
//   Left 25% = LEFT, Right 25% = RIGHT
//   Center = CONFIRM
#define DIR_MARGIN 60  // pixels from edge for directional zones

// --- State ---
static unsigned long s_lastTouchMs = 0;
static bool s_wasTouched = false;
static int s_lastX = 0, s_lastY = 0;

void touchInputBegin() {
    s_lastTouchMs = millis();
    s_wasTouched = false;
}

InputState touchInputPoll(XPT2046_Touchscreen &touch) {
    InputState in = {};
    in.tabDirect = -1;

    bool touched = touch.touched();

    if (touched) {
        TS_Point p = touch.getPoint();
        int tx = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_WIDTH);
        int ty = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_HEIGHT);
        tx = clampi(tx, 0, SCREEN_WIDTH - 1);
        ty = clampi(ty, 0, SCREEN_HEIGHT - 1);

        s_lastX = tx;
        s_lastY = ty;
        s_lastTouchMs = millis();
        in.anyTouch = true;
        in.touchX = tx;
        in.touchY = ty;
        in.freshPress = !s_wasTouched;

        // --- Tab bar detection ---
        if (ty >= TAB_BAR_Y) {
            int tabWidth = SCREEN_WIDTH / TAB_COUNT;
            int tabIdx = tx / tabWidth;
            if (tabIdx >= TAB_COUNT) tabIdx = TAB_COUNT - 1;
            in.tabDirect = tabIdx;
        }
        // --- Content area directional input ---
        else {
            // Divide content area into zones
            if (ty < DIR_MARGIN) {
                in.up = true;
            } else if (ty > (CONTENT_HEIGHT - DIR_MARGIN)) {
                in.down = true;
            }

            if (tx < DIR_MARGIN) {
                in.left = true;
            } else if (tx > (SCREEN_WIDTH - DIR_MARGIN)) {
                in.right = true;
            }

            // Center zone = confirm
            bool inCenterX = (tx > DIR_MARGIN && tx < (SCREEN_WIDTH - DIR_MARGIN));
            bool inCenterY = (ty > DIR_MARGIN && ty < (CONTENT_HEIGHT - DIR_MARGIN));
            if (inCenterX && inCenterY) {
                in.confirm = true;
            }

            // Back button: top-left corner (50x30)
            if (tx < 50 && ty < 30) {
                in.back = true;
                in.confirm = false;
                in.up = false;
                in.left = false;
            }
        }

        s_wasTouched = true;
    } else {
        s_wasTouched = false;
    }

    return in;
}

int touchLastX() { return s_lastX; }
int touchLastY() { return s_lastY; }

unsigned long touchIdleMs() {
    return millis() - s_lastTouchMs;
}

void touchResetIdle() {
    s_lastTouchMs = millis();
}
