#pragma once
#include <Arduino.h>
#include "game_types.h"

struct Item {
    ItemType type;
    uint8_t  quantity;
};

// Get item display name (themed per pet type)
const char* itemName(ItemType type, PetType petType);
const char* itemDesc(ItemType type);

// Stat effect preview
struct ItemDeltas {
    int hunger, happiness, energy, health, xp;
};
ItemDeltas itemPreviewDeltas(ItemType type);

class Inventory {
public:
    static constexpr int MAX_ITEMS = MAX_INV_ITEMS;

    Inventory();

    Item items[MAX_ITEMS];
    int  selectedIndex;

    void init();
    void resetToDefaults();
    void clear();

    bool addItem(ItemType type, int qty);
    bool removeItem(ItemType type, int qty);
    bool hasItem(ItemType type) const;
    int  getQuantity(ItemType type) const;
    int  countItems() const;

    // Use the selected item on the pet (applies stat effects)
    // Returns true if item was used successfully
    bool useSelectedItem();
    bool useItem(ItemType type);

    // Persistence
    void toPersist(InvPersist &out) const;
    void fromPersist(const InvPersist &in);

    // Find first slot with this type, or -1
    int findSlot(ItemType type) const;
    // Find first empty slot, or -1
    int findEmptySlot() const;
    // Get visible item list (non-empty slots)
    int getVisibleCount() const;
    ItemType getVisibleType(int visIdx) const;
};

// Global inventory
extern Inventory inventory;
