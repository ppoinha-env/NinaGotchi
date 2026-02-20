#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// --- HARDWARE PINS ---
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define TFT_BL 21

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite faceSprite = TFT_eSprite(&tft);
SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SPRITE_W 220 
#define SPRITE_H 220 

// --- COLORS ---
#define COLOR_PINK 0xFC18 
#define COLOR_BROWN 0x9300
#define COLOR_MIDNIGHT 0x18FF
#define COLOR_PURPLE 0x780F
#define COLOR_ORANGE 0xFD20
#define COLOR_SKY 0x5DFF
#define COLOR_LEAF 0x07E0
#define COLOR_COOKIE 0xFD20
#define COLOR_CHIP 0x4208

// --- MODES & TIMING ---
enum AppMode { PET_MODE, LEARNING_MODE };
AppMode currentAppMode = PET_MODE;

unsigned long lastTouchTime = 0;
const unsigned long SLEEP_THRESHOLD = 300000; 
unsigned long lastMoodChange = 0;
const unsigned long MOOD_INTERVAL = 10000;    
unsigned long lastValueChange = 0;
unsigned long animEndTime = 0; 
unsigned long feedingStartTime = 0; 
String currentLearningValue = "A";

bool isPetting = false, isDizzy = false, isFeeding = false, isSleeping = false;
int currentMood = 0, cookieX = 0;
float breathScale = 1.0, breathDir = 0.003;

// --- DRAWING HELPERS ---

void drawHeart(TFT_eSprite &sp, int x, int y, int size, uint16_t color) {
  sp.fillSmoothCircle(x - size/2, y, size/2, color);
  sp.fillSmoothCircle(x + size/2, y, size/2, color);
  sp.fillTriangle(x - size, y + size/4, x + size, y + size/4, x, y + size, color);
}

void drawSidebar(int activeBtn) {
  int btnH = 60; 
  tft.setTextDatum(MC_DATUM);
  tft.fillRect(0, 0, 60, 240, 0x2104); 
  if (activeBtn == 1) tft.fillRect(0, 0, 60, btnH, TFT_WHITE);
  uint16_t hCol = (activeBtn == 1) ? TFT_BLACK : TFT_RED;
  tft.fillSmoothCircle(23, 25, 7, hCol);
  tft.fillSmoothCircle(37, 25, 7, hCol);
  tft.fillTriangle(16, 28, 44, 28, 30, 45, hCol);
  if (activeBtn == 2) tft.fillRect(0, btnH, 60, btnH, TFT_WHITE);
  tft.fillSmoothCircle(30, 90, 15, COLOR_BROWN);
  tft.fillCircle(25, 85, 2, TFT_BLACK); 
  tft.fillCircle(35, 92, 2, TFT_BLACK);
  tft.fillCircle(28, 98, 2, TFT_BLACK);
  if (activeBtn == 3) tft.fillRect(0, btnH*2, 60, btnH, TFT_WHITE);
  uint16_t sCol = (activeBtn == 3) ? COLOR_PINK : TFT_WHITE;
  for(int i=0; i<3; i++) {
    tft.drawSmoothArc(30, 150, 16-i*4, 14-i*4, 0, 270, sCol, (activeBtn == 3) ? TFT_WHITE : 0x2104);
  }
  tft.fillRect(0, btnH * 3, 60, btnH, (activeBtn == 4) ? TFT_YELLOW : 0x0186);
  tft.setTextColor(currentAppMode == LEARNING_MODE ? TFT_RED : TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString(currentAppMode == LEARNING_MODE ? "X" : "1/A", 30, 210);
  tft.drawFastVLine(60, 0, 240, TFT_DARKGREY);
}

void generateRandomValue() {
  if (random(0, 100) < 50) currentLearningValue = String(random(0, 11));
  else currentLearningValue = String((char)random(65, 91));
}

void drawFace(float scale, bool smallMode) {
  faceSprite.fillSprite(TFT_BLACK);
  int cx = SPRITE_W / 2; int cy = SPRITE_H / 2;
  float finalScale = smallMode ? 0.6 : scale;
  int drawX = smallMode ? 50 : cx;
  int drawY = cy;
  
  bool pettingActive = (millis() < animEndTime && isPetting);
  bool dizzyActive = (millis() < animEndTime && isDizzy);
  
  float subAnim = (float)(millis() % 2000) / 2000.0;
  bool blinkState = (millis() % 3000 < 200);

  uint16_t moodColor = COLOR_PINK;
  if (isSleeping) moodColor = COLOR_MIDNIGHT;
  else if (pettingActive) moodColor = TFT_MAGENTA;
  else if (dizzyActive) moodColor = TFT_WHITE;
  else if (isFeeding) moodColor = COLOR_ORANGE; 
  else {
    uint16_t palette[] = {COLOR_PINK, TFT_YELLOW, COLOR_PURPLE, COLOR_ORANGE, TFT_CYAN, COLOR_LEAF, TFT_RED, COLOR_SKY, TFT_WHITE, COLOR_BROWN, 0xFC18, 0xFD20, 0x780F, 0x07E0, 0x5DFF, 0xFC18, 0xFFFF, 0x9300, 0xFC18, 0x780F};
    moodColor = palette[currentMood % 20];
  }

  faceSprite.drawSmoothArc(drawX, drawY, 100*finalScale, 96*finalScale, 0, 360, moodColor, TFT_BLACK);

  int eyeSp = 45 * finalScale;
  int eyeY = drawY - (10 * finalScale);
  int mouthY = drawY + (40 * finalScale);

  if (isSleeping) {
    faceSprite.drawSmoothArc(drawX - eyeSp, eyeY, 15*finalScale, 12*finalScale, 100, 260, moodColor, TFT_BLACK);
    faceSprite.drawSmoothArc(drawX + eyeSp, eyeY, 15*finalScale, 12*finalScale, 100, 260, moodColor, TFT_BLACK);
    faceSprite.fillSmoothCircle(drawX + 60, drawY - 60, 15, TFT_YELLOW);
    faceSprite.fillSmoothCircle(drawX + 53, drawY - 60, 12, TFT_BLACK);
    if ((millis() / 400) % 2 == 0) faceSprite.fillSmoothCircle(drawX - 70, drawY - 50, 2, TFT_WHITE);
    for (int i = 0; i < 3; i++) {
      float zOffset = (float)((millis() + (i * 800)) % 2400) / 2400.0; 
      int zPosX = drawX + 30 + (zOffset * 40); int zPosY = drawY - 20 - (zOffset * 60);
      int zSize = 3 - (zOffset * 2);
      if (zSize > 0) { faceSprite.setTextSize(zSize); faceSprite.setTextColor(TFT_WHITE); faceSprite.drawString("Z", zPosX, zPosY); }
    }
    faceSprite.drawSmoothArc(drawX, mouthY, 8 * finalScale, 6 * finalScale, 0, 360, moodColor, TFT_BLACK);
  } else if (pettingActive) {
    float hPulse = 1.0 + (sin(millis()*0.01)*0.1);
    drawHeart(faceSprite, drawX - eyeSp, eyeY, 14*hPulse*finalScale, moodColor);
    drawHeart(faceSprite, drawX + eyeSp, eyeY, 14*hPulse*finalScale, moodColor);
    faceSprite.fillSmoothCircle(drawX, mouthY, 20 * finalScale, moodColor);
    faceSprite.fillRect(drawX - 30, mouthY - 20, 60, 20, TFT_BLACK);
  } else if (dizzyActive) {
    // --- DIZZY ANIMATION (SHAKE BUTTON) ---
    int dOffset = sin(millis()*0.02)*5;
    faceSprite.drawCircle(drawX - eyeSp, eyeY + dOffset, 10*finalScale, moodColor);
    faceSprite.drawCircle(drawX + eyeSp, eyeY - dOffset, 10*finalScale, moodColor);
    faceSprite.drawSmoothArc(drawX, mouthY, 15*finalScale, 13*finalScale, 0, 360, moodColor, TFT_BLACK);
    
    // --- FLOATING "AGH" ANIMATION ---
    float progress = (float)(animEndTime - millis()) / 3000.0; // 1.0 to 0.0
    int floatY = mouthY - map((int)(progress * 100), 0, 100, 80, 20); // Move up
    int tSize = (progress > 0.5) ? 2 : 1; // Get smaller half-way
    uint16_t fadeCol = (progress > 0.6) ? TFT_WHITE : (progress > 0.3 ? TFT_SILVER : TFT_DARKGREY);
    
    faceSprite.setTextColor(fadeCol);
    faceSprite.setTextSize(tSize);
    faceSprite.setTextDatum(MC_DATUM);
    faceSprite.drawString("Aghhhh !!!", drawX + dOffset, floatY);
  } else if (isFeeding) {
    faceSprite.fillSmoothCircle(drawX - eyeSp, eyeY, 12 * finalScale, moodColor);
    faceSprite.fillSmoothCircle(drawX + eyeSp, eyeY, 12 * finalScale, moodColor);
    
    if (cookieX < drawX + 10) {
      float distToMouth = abs(drawX - cookieX);
      int cookieSize = (distToMouth < 40) ? map(distToMouth, 0, 40, 6, 18) : 18;
      faceSprite.fillSmoothCircle(cookieX, mouthY, cookieSize, COLOR_COOKIE);
      if (cookieSize > 8) {
        faceSprite.fillCircle(cookieX-2, mouthY-2, 2, COLOR_CHIP);
        faceSprite.fillCircle(cookieX+2, mouthY+2, 2, COLOR_CHIP);
      }
      int mouthOpen = 8 + (sin(millis() * 0.015) * 6);
      faceSprite.drawSmoothArc(drawX, mouthY, mouthOpen, mouthOpen - 3, 0, 360, moodColor, TFT_BLACK);
      cookieX += 3; 
    } else {
      faceSprite.drawSmoothArc(drawX, mouthY, 12, 9, 180, 360, moodColor, TFT_BLACK);
      faceSprite.setTextColor(TFT_WHITE);
      faceSprite.setTextSize(2);
      faceSprite.drawString("Yumm !", drawX + 45, mouthY - 25);
      if (millis() - feedingStartTime > 5000) isFeeding = false; 
    }
  } else {
    switch(currentMood) {
      case 1: 
        {
          for(int i=0; i<360; i+=45) {
            int rot = millis()/20;
            faceSprite.drawFastVLine(drawX + cos((i+rot)*0.017)*75, drawY + sin((i+rot)*0.017)*75, 12, TFT_YELLOW);
          }
          if(!blinkState) { faceSprite.fillSmoothCircle(drawX-eyeSp, eyeY, 12, moodColor); faceSprite.fillSmoothCircle(drawX+eyeSp, eyeY, 12, moodColor); }
          faceSprite.fillSmoothCircle(drawX, mouthY, 20 * finalScale, moodColor);
          faceSprite.fillRect(drawX - 25, mouthY - 20, 50, 20, TFT_BLACK);
        }
        break;
      case 2: 
        {
          int twitch = sin(millis()*0.03)*5;
          faceSprite.fillTriangle(drawX-60, drawY-80, drawX-30, drawY-100, drawX-10, drawY-80, moodColor);
          faceSprite.fillTriangle(drawX+60, drawY-80, drawX+30, drawY-100, drawX+10, drawY-80, moodColor);
          faceSprite.drawLine(drawX-20, drawY, drawX-50+twitch, drawY-10, moodColor);
          faceSprite.drawLine(drawX+20, drawY, drawX+50-twitch, drawY-10, moodColor);
          faceSprite.fillSmoothCircle(drawX-eyeSp, eyeY, 10, moodColor); faceSprite.fillSmoothCircle(drawX+eyeSp, eyeY, 10, moodColor);
          faceSprite.drawSmoothArc(drawX-8, mouthY, 10, 8, 100, 260, moodColor, TFT_BLACK);
          faceSprite.drawSmoothArc(drawX+8, mouthY, 10, 8, 100, 260, moodColor, TFT_BLACK);
        }
        break;
      case 3: 
        {
          faceSprite.fillSmoothCircle(drawX, drawY-85, 20, TFT_LIGHTGREY);
          for(int i=0; i<3; i++) {
            int rx = drawX - 20 + (i*20);
            int ry = drawY - 60 + ((millis() + i*500) % 40);
            faceSprite.fillCircle(rx, ry, 2, COLOR_SKY);
          }
          faceSprite.drawSmoothArc(drawX-eyeSp, eyeY, 12, 10, 100, 260, moodColor, TFT_BLACK);
          faceSprite.drawSmoothArc(drawX, mouthY+10, 15, 13, 100, 260, moodColor, TFT_BLACK);
        }
        break;
      case 4: 
        {
          int v = random(-2, 3);
          faceSprite.drawLine(drawX-eyeSp-10, eyeY-15+v, drawX-eyeSp+10, eyeY-5+v, moodColor);
          faceSprite.drawLine(drawX+eyeSp+10, eyeY-15+v, drawX+eyeSp-10, eyeY-5+v, moodColor);
          faceSprite.fillSmoothCircle(drawX-eyeSp, eyeY+v, 10, moodColor);
          faceSprite.fillSmoothCircle(drawX+eyeSp, eyeY+v, 10, moodColor);
          faceSprite.drawFastHLine(drawX-15+v, mouthY, 30, moodColor);
        }
        break;
      case 6: 
        {
          faceSprite.fillSmoothCircle(drawX-eyeSp, eyeY, 12, moodColor);
          if (subAnim < 0.5) faceSprite.fillSmoothCircle(drawX+eyeSp, eyeY, 12, moodColor);
          else faceSprite.drawFastHLine(drawX+eyeSp-10, eyeY, 20, moodColor);
          faceSprite.drawSmoothArc(drawX+5, mouthY, 15, 13, 180, 280, moodColor, TFT_BLACK);
        }
        break;
      case 11: 
        {
          faceSprite.fillSmoothCircle(drawX-eyeSp, eyeY, 10, moodColor); faceSprite.fillSmoothCircle(drawX+eyeSp, eyeY, 10, moodColor);
          faceSprite.fillSmoothCircle(drawX+70, drawY-40+((millis()/10)%30), 4, COLOR_SKY);
          faceSprite.drawCircle(drawX, mouthY, 6, moodColor);
        }
        break;
      default: 
        {
          faceSprite.fillSmoothCircle(drawX-eyeSp, eyeY, 15*finalScale, moodColor);
          faceSprite.fillSmoothCircle(drawX+eyeSp, eyeY, 15*finalScale, moodColor);
          float smile = sin(millis()*0.002);
          if (smile > 0) {
            faceSprite.fillSmoothCircle(drawX, mouthY, 15 * finalScale * smile, moodColor);
            faceSprite.fillRect(drawX - 25, mouthY - 18, 50, 18, TFT_BLACK);
          } else {
            faceSprite.drawFastHLine(drawX-15, mouthY, 30, moodColor);
          }
        }
        break;
    }
    if(currentMood > 6 && currentMood != 11) {
       int mSize = 10 + sin(millis()*0.005)*8;
       faceSprite.fillSmoothCircle(drawX, mouthY, mSize * finalScale, moodColor);
       faceSprite.fillRect(drawX - 25, mouthY - 18, 50, 18, TFT_BLACK);
    }
  }

  if (smallMode) {
    faceSprite.setTextColor(TFT_WHITE, TFT_BLACK); faceSprite.setTextDatum(MC_DATUM);
    faceSprite.setTextSize(4); faceSprite.drawString(currentLearningValue, 150, cy, 4); 
  }

  int shake = (dizzyActive && !isSleeping) ? random(-6, 6) : 0;
  faceSprite.pushSprite(75 + shake, (SCREEN_HEIGHT - SPRITE_H) / 2);
}

void setup() {
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
  tft.init(); tft.setRotation(1); tft.invertDisplay(true); tft.fillScreen(TFT_BLACK);
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touch.begin(touchSPI); touch.setRotation(1);
  faceSprite.createSprite(SPRITE_W, SPRITE_H);
  faceSprite.setTextDatum(MC_DATUM);
  tft.fillScreen(TFT_BLACK);
  drawSidebar(0);
  lastTouchTime = millis();
}

void loop() {
  unsigned long now = millis();
  static int lastBtn = 0;
  int currentActiveBtn = 0;

  if (touch.touched()) {
    if (isSleeping) { isSleeping = false; tft.fillRect(61, 0, 259, 240, TFT_BLACK); }
    lastTouchTime = now;
    TS_Point p = touch.getPoint();
    int tx = map(p.x, 200, 3700, 0, SCREEN_WIDTH);
    int ty = map(p.y, 240, 3800, 0, SCREEN_HEIGHT);
    if (tx < 60) { 
      if (ty < 60) { 
        isPetting = true; 
        isDizzy = false; 
        animEndTime = now + 3000; 
        currentActiveBtn = 1; 
      }
      else if (ty < 120) { 
        if(!isFeeding) { 
          isFeeding = true; 
          cookieX = 0; 
          feedingStartTime = now; 
        } 
        currentActiveBtn = 2; 
      }
      else if (ty < 180) { 
        isDizzy = true; 
        isPetting = false; 
        animEndTime = now + 3000; 
        currentActiveBtn = 3; 
      }
      else { 
        if (lastBtn != 4) {
          currentAppMode = (currentAppMode == PET_MODE) ? LEARNING_MODE : PET_MODE;
          if (currentAppMode == LEARNING_MODE) { generateRandomValue(); lastValueChange = now; }
          tft.fillRect(61, 0, 259, 240, TFT_BLACK);
        }
        currentActiveBtn = 4;
      }
      if (currentActiveBtn != lastBtn) { drawSidebar(currentActiveBtn); lastBtn = currentActiveBtn; }
    }
  } else {
    if (lastBtn != 0) { drawSidebar(0); lastBtn = 0; }
  }

  if (now > animEndTime) {
    isPetting = false;
    isDizzy = false;
  }

  if (!isSleeping && (now - lastTouchTime > SLEEP_THRESHOLD)) { 
    isSleeping = true; 
    tft.fillRect(61, 0, 259, 240, TFT_BLACK); 
  }

  if (!isSleeping && currentAppMode == PET_MODE && (now - lastMoodChange > MOOD_INTERVAL)) {
    currentMood = random(0, 20); 
    lastMoodChange = now;
  }

  if (currentAppMode == LEARNING_MODE && !isSleeping && (now - lastValueChange > 3000)) {
    generateRandomValue(); lastValueChange = now;
  }

  breathScale += isSleeping ? (breathDir * 0.5) : breathDir;
  if (breathScale > 1.1 || breathScale < 0.9) breathDir *= -1;
  drawFace(breathScale, (currentAppMode == LEARNING_MODE));
  delay(20);
}
