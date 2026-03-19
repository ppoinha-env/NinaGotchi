#pragma once
#include <Arduino.h>
#include "game_types.h"

class Pet {
public:
    Pet();

    // Identity
    char     name[PET_NAME_MAX + 1];
    PetType  type;
    uint8_t  evoStage;   // 0=baby, 1=teen, 2=adult, 3=elder

    // Core stats (0-100)
    int hunger;
    int happiness;
    int energy;
    int health;

    // Economy / progression
    int32_t  inf;        // Inferium currency
    uint16_t level;
    uint32_t xp;

    // Sleep state
    bool isSleeping;
    uint32_t sleepStartMs;

    // Timing accumulators (ms since last tick)
    uint32_t hungerAccum;
    uint32_t energyAccum;
    uint32_t hpAccum;
    uint32_t moodAccum;
    uint32_t hpRegenAccum;
    uint32_t lastUpdateMs;

    // Birth time (millis at creation)
    uint32_t birthMs;

    // --- Methods ---
    void setName(const char* n);
    const char* getName() const { return name; }

    // Mood resolution (priority: sick > tired > hungry > mad > bored > happy)
    PetMood getMood() const;

    // Main update — call every frame, handles stat decay
    void update(DecayMode mode);

    // Sleep system
    void startSleep();
    void wakeUp();
    void sleepTick();

    // Sleep quality (0-100 based on mood/hunger/health)
    uint8_t getSleepQuality() const;

    // Stat modifications
    void feed(int amount);
    void heal(int amount);
    void tire(int amount);
    void makeHappy(int amount);
    void clampStats();

    // XP / Level / Evolution
    void   addXP(uint32_t amount);
    uint32_t xpForNextLevel() const;
    bool   canEvolve() const;
    uint16_t nextEvoMinLevel() const;
    bool   tryEvolve();  // Requires Eldritch Eye + level threshold + mood
    void   setEvoStage(uint8_t stage);
    const char* getEvoDescriptor() const;

    // Death check
    bool isDead() const { return health <= 0; }

    // Persistence
    void toPersist(PetPersist &out) const;
    void fromPersist(const PetPersist &in);

    // Reset to defaults
    void init();

    // Age string
    void getAgeString(char* out, size_t outSize) const;

    // Build display name "Stage Type Name"
    void buildDisplayName(char* out, size_t outSize) const;
};

// Global pet instance
extern Pet pet;

// Currency helpers (stored in pet.inf)
int  getInf();
void addInf(int amount);
bool spendInf(int amount);
