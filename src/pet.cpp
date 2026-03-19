#include "pet.h"

Pet pet;

// ============================================================
// Constructor / Init
// ============================================================

Pet::Pet() { init(); }

void Pet::init() {
    memset(name, 0, sizeof(name));
    strcpy(name, "Baby");
    type = PET_DEVIL;
    evoStage = EVO_BABY;
    hunger = 50;
    happiness = 50;
    energy = 50;
    health = 100;
    inf = 0;
    level = 1;
    xp = 0;
    isSleeping = false;
    sleepStartMs = 0;
    hungerAccum = 0;
    energyAccum = 0;
    hpAccum = 0;
    moodAccum = 0;
    hpRegenAccum = 0;
    lastUpdateMs = millis();
    birthMs = millis();
}

void Pet::setName(const char* n) {
    strncpy(name, n, PET_NAME_MAX);
    name[PET_NAME_MAX] = '\0';
}

// ============================================================
// Mood Resolution — Priority order
// ============================================================

PetMood Pet::getMood() const {
    if (health <= 60)    return MOOD_SICK;
    if (energy <= 30)    return MOOD_TIRED;
    if (hunger <= 30)    return MOOD_HUNGRY;
    if (happiness <= 30) return MOOD_MAD;
    if (happiness < 60)  return MOOD_BORED;
    return MOOD_HAPPY;
}

// ============================================================
// Decay Scaling Helper
// ============================================================

static uint32_t applyDecayScale(uint32_t baseMs, DecayMode mode) {
    // Higher multiplier = faster decay = shorter interval
    uint16_t mult = decayMultiplierQ8(mode);
    // interval = baseMs * 256 / mult
    if (mult == 0) return baseMs;
    return (uint32_t)((uint64_t)baseMs * 256ULL / (uint64_t)mult);
}

// ============================================================
// Main Update — Real-time stat decay
// ============================================================

void Pet::update(DecayMode mode) {
    if (isDead()) return;

    uint32_t now = millis();
    uint32_t dt = now - lastUpdateMs;
    lastUpdateMs = now;

    if (isSleeping) {
        // Sleep ticks handled separately
        return;
    }

    // Accumulate time
    hungerAccum  += dt;
    energyAccum  += dt;
    hpAccum      += dt;
    moodAccum    += dt;
    hpRegenAccum += dt;

    // --- Hunger decay ---
    uint32_t hungerInterval = applyDecayScale(HUNGER_STEP_MS, mode);
    while (hungerAccum >= hungerInterval) {
        hungerAccum -= hungerInterval;
        hunger--;
    }

    // --- Energy decay ---
    uint32_t energyInterval = applyDecayScale(ENERGY_STEP_MS, mode);
    while (energyAccum >= energyInterval) {
        energyAccum -= energyInterval;
        energy--;
    }

    // --- Health decay (only when starving: hunger <= 0) ---
    if (hunger <= 0) {
        uint32_t hpInterval = applyDecayScale(HP_STEP_MS, mode);
        while (hpAccum >= hpInterval) {
            hpAccum -= hpInterval;
            health--;
        }
    } else {
        hpAccum = 0;
    }

    // --- Mood / Happiness decay ---
    PetMood m = getMood();
    uint32_t moodInterval = (m == MOOD_HAPPY)
        ? applyDecayScale(MOOD_HAPPY_MS, mode)
        : applyDecayScale(MOOD_DOWN_MS, mode);
    while (moodAccum >= moodInterval) {
        moodAccum -= moodInterval;
        happiness--;
    }

    // --- Health regen (when fed and rested) ---
    if (hunger >= 30 && energy > 15 && health < 100) {
        uint32_t regenInterval = applyDecayScale(HP_REGEN_MS, mode);
        while (hpRegenAccum >= regenInterval) {
            hpRegenAccum -= regenInterval;
            health++;
        }
    } else {
        hpRegenAccum = 0;
    }

    clampStats();
}

// ============================================================
// Sleep System
// ============================================================

void Pet::startSleep() {
    isSleeping = true;
    sleepStartMs = millis();
}

void Pet::wakeUp() {
    isSleeping = false;
}

uint8_t Pet::getSleepQuality() const {
    // Quality based on hunger, mood, health (higher = better)
    int score = 0;
    score += clampi(hunger, 0, 100);       // 0-100
    score += clampi(health, 0, 100);       // 0-100
    if (getMood() == MOOD_HAPPY) score += 50;
    else if (getMood() == MOOD_BORED) score += 25;
    // score range: 0-250, normalize to 0-100
    return (uint8_t)clampi(score * 100 / 250, 0, 100);
}

void Pet::sleepTick() {
    if (!isSleeping) return;

    // Energy recovery based on sleep quality
    uint8_t quality = getSleepQuality();
    // Quality maps recovery: 0-33 = +1, 34-66 = +2, 67-100 = +3
    int recovery = 1;
    if (quality > 66) recovery = 3;
    else if (quality > 33) recovery = 2;

    energy += recovery;

    // Slow happiness recovery
    happiness += 1;

    // Slow hunger drain during sleep (floor at 25)
    if (hunger > 25) hunger -= 1;

    // Health regen if not starving
    if (hunger >= 30) health += 1;

    clampStats();

    // Auto-wake when fully rested
    if (energy >= 100) {
        wakeUp();
    }
}

// ============================================================
// Stat Modifications
// ============================================================

void Pet::feed(int amount) {
    hunger += amount;
    clampStats();
}

void Pet::heal(int amount) {
    health += amount;
    clampStats();
}

void Pet::tire(int amount) {
    energy -= amount;
    clampStats();
}

void Pet::makeHappy(int amount) {
    happiness += amount;
    clampStats();
}

void Pet::clampStats() {
    hunger    = clampi(hunger, 0, 100);
    happiness = clampi(happiness, 0, 100);
    energy    = clampi(energy, 0, 100);
    health    = clampi(health, 0, 100);
    if (inf < 0) inf = 0;
    if (inf > 999999) inf = 999999;
}

// ============================================================
// XP / Level / Evolution
// ============================================================

void Pet::addXP(uint32_t amount) {
    xp += amount;
    while (xp >= xpForNextLevel() && level < 999) {
        xp -= xpForNextLevel();
        level++;
    }
}

uint32_t Pet::xpForNextLevel() const {
    return XP_BASE + (uint32_t)(level - 1) * XP_PER_LEVEL;
}

uint16_t Pet::nextEvoMinLevel() const {
    switch (evoStage) {
        case EVO_BABY:  return EVO_TEEN_LEVEL;
        case EVO_TEEN:  return EVO_ADULT_LEVEL;
        case EVO_ADULT: return EVO_ELDER_LEVEL;
        default:        return 999;
    }
}

bool Pet::canEvolve() const {
    if (evoStage >= EVO_ELDER) return false;
    if (level < nextEvoMinLevel()) return false;
    PetMood m = getMood();
    return (m == MOOD_HAPPY || m == MOOD_BORED);
}

bool Pet::tryEvolve() {
    if (!canEvolve()) return false;
    evoStage++;
    return true;
}

void Pet::setEvoStage(uint8_t stage) {
    evoStage = clampi(stage, 0, EVO_ELDER);
}

const char* Pet::getEvoDescriptor() const {
    static char buf[32];
    snprintf(buf, sizeof(buf), "%s %s", evoStageName((EvoStage)evoStage), petTypeName(type));
    return buf;
}

// ============================================================
// Persistence
// ============================================================

void Pet::toPersist(PetPersist &out) const {
    memset(&out, 0, sizeof(out));
    out.hunger     = (uint8_t)clampi(hunger, 0, 255);
    out.happiness  = (uint8_t)clampi(happiness, 0, 255);
    out.energy     = (uint8_t)clampi(energy, 0, 255);
    out.health     = (uint8_t)clampi(health, 0, 255);
    out.petType    = (uint8_t)type;
    out.isSleeping = isSleeping ? 1 : 0;
    out.evoStage   = evoStage;
    out.level      = level;
    out.xp         = xp;
    out.inf        = inf;
    out.birthEpoch = birthMs;
    strncpy(out.name, name, PET_NAME_MAX);
    out.name[PET_NAME_MAX] = '\0';
}

void Pet::fromPersist(const PetPersist &in) {
    hunger     = in.hunger;
    happiness  = in.happiness;
    energy     = in.energy;
    health     = in.health;
    type       = (PetType)in.petType;
    isSleeping = (in.isSleeping != 0);
    evoStage   = in.evoStage;
    level      = in.level;
    xp         = in.xp;
    inf        = in.inf;
    birthMs    = in.birthEpoch;
    strncpy(name, in.name, PET_NAME_MAX);
    name[PET_NAME_MAX] = '\0';

    // Reset accumulators
    hungerAccum = 0;
    energyAccum = 0;
    hpAccum = 0;
    moodAccum = 0;
    hpRegenAccum = 0;
    lastUpdateMs = millis();
    sleepStartMs = isSleeping ? millis() : 0;
}

// ============================================================
// Age
// ============================================================

void Pet::getAgeString(char* out, size_t outSize) const {
    uint32_t elapsedMs = millis() - birthMs;
    uint32_t secs = elapsedMs / 1000;
    uint32_t mins = secs / 60;
    uint32_t hrs  = mins / 60;
    uint32_t days = hrs / 24;

    if (days > 0) {
        snprintf(out, outSize, "%lud %luh", (unsigned long)days, (unsigned long)(hrs % 24));
    } else if (hrs > 0) {
        snprintf(out, outSize, "%luh %lum", (unsigned long)hrs, (unsigned long)(mins % 60));
    } else {
        snprintf(out, outSize, "%lum", (unsigned long)mins);
    }
}

void Pet::buildDisplayName(char* out, size_t outSize) const {
    snprintf(out, outSize, "%s %s \"%s\"",
             evoStageName((EvoStage)evoStage),
             petTypeName(type),
             name);
}

// ============================================================
// Currency (stored in pet.inf)
// ============================================================

int getInf() { return pet.inf; }

void addInf(int amount) {
    long v = (long)pet.inf + (long)amount;
    if (v < 0) v = 0;
    if (v > 999999) v = 999999;
    pet.inf = (int)v;
}

bool spendInf(int amount) {
    if (amount <= 0) return true;
    if (pet.inf < amount) return false;
    pet.inf -= amount;
    return true;
}
