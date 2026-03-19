#include "inventory.h"
#include "pet.h"

Inventory inventory;

Inventory::Inventory() { init(); }

void Inventory::init() {
    clear();
    resetToDefaults();
}

void Inventory::resetToDefaults() {
    clear();
    addItem(ITEM_SOUL_FOOD, 3);
    addItem(ITEM_CURSED_RELIC, 1);
    addItem(ITEM_DEMON_BONE, 1);
}

void Inventory::clear() {
    for (int i = 0; i < MAX_ITEMS; i++) {
        items[i].type = ITEM_NONE;
        items[i].quantity = 0;
    }
    selectedIndex = 0;
}

int Inventory::findSlot(ItemType type) const {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (items[i].type == type && items[i].quantity > 0) return i;
    }
    return -1;
}

int Inventory::findEmptySlot() const {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (items[i].type == ITEM_NONE || items[i].quantity == 0) return i;
    }
    return -1;
}

bool Inventory::addItem(ItemType type, int qty) {
    if (type == ITEM_NONE || qty <= 0) return false;

    // Try to stack in existing slot first
    int slot = findSlot(type);
    if (slot >= 0) {
        items[slot].quantity += qty;
        if (items[slot].quantity > 99) items[slot].quantity = 99;
        return true;
    }

    // Find empty slot
    slot = findEmptySlot();
    if (slot < 0) return false;  // Inventory full

    items[slot].type = type;
    items[slot].quantity = qty;
    return true;
}

bool Inventory::removeItem(ItemType type, int qty) {
    int slot = findSlot(type);
    if (slot < 0) return false;
    if (items[slot].quantity < (uint8_t)qty) return false;

    items[slot].quantity -= qty;
    if (items[slot].quantity == 0) {
        items[slot].type = ITEM_NONE;
    }
    return true;
}

bool Inventory::hasItem(ItemType type) const {
    return findSlot(type) >= 0;
}

int Inventory::getQuantity(ItemType type) const {
    int slot = findSlot(type);
    if (slot < 0) return 0;
    return items[slot].quantity;
}

int Inventory::countItems() const {
    int count = 0;
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (items[i].type != ITEM_NONE && items[i].quantity > 0) count++;
    }
    return count;
}

int Inventory::getVisibleCount() const {
    return countItems();
}

ItemType Inventory::getVisibleType(int visIdx) const {
    int seen = 0;
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (items[i].type != ITEM_NONE && items[i].quantity > 0) {
            if (seen == visIdx) return items[i].type;
            seen++;
        }
    }
    return ITEM_NONE;
}

// ============================================================
// Item Effects
// ============================================================

ItemDeltas itemPreviewDeltas(ItemType type) {
    ItemDeltas d = {0, 0, 0, 0, 0};
    switch (type) {
        case ITEM_SOUL_FOOD:    d.hunger = 30; d.happiness = 10; d.energy = 10; break;
        case ITEM_CURSED_RELIC: d.happiness = 30; break;
        case ITEM_DEMON_BONE:   d.energy = 30; break;
        case ITEM_RITUAL_CHALK: d.health = 30; break;
        case ITEM_ELDRITCH_EYE: break;  // Evolution trigger, no direct stat effect
        default: break;
    }
    return d;
}

bool Inventory::useItem(ItemType type) {
    if (!hasItem(type)) return false;

    if (type == ITEM_ELDRITCH_EYE) {
        // Evolution trigger
        if (!pet.canEvolve()) return false;
        removeItem(type, 1);
        return true;  // Caller should handle evolution flow
    }

    ItemDeltas d = itemPreviewDeltas(type);
    pet.hunger    += d.hunger;
    pet.happiness += d.happiness;
    pet.energy    += d.energy;
    pet.health    += d.health;
    pet.clampStats();
    removeItem(type, 1);
    return true;
}

bool Inventory::useSelectedItem() {
    ItemType type = getVisibleType(selectedIndex);
    if (type == ITEM_NONE) return false;
    return useItem(type);
}

// ============================================================
// Persistence
// ============================================================

void Inventory::toPersist(InvPersist &out) const {
    memset(&out, 0, sizeof(out));
    for (int i = 0; i < MAX_ITEMS; i++) {
        out.slots[i].type = (uint8_t)items[i].type;
        out.slots[i].qty  = items[i].quantity;
    }
}

void Inventory::fromPersist(const InvPersist &in) {
    for (int i = 0; i < MAX_ITEMS; i++) {
        items[i].type     = (ItemType)in.slots[i].type;
        items[i].quantity = in.slots[i].qty;
    }
    selectedIndex = 0;
}

// ============================================================
// Item Names — themed per pet type
// ============================================================

const char* itemName(ItemType type, PetType petType) {
    switch (type) {
        case ITEM_SOUL_FOOD:
            switch (petType) {
                case PET_DEVIL:    return "Soul Food";
                case PET_ELDRITCH: return "Brine Bites";
                case PET_ALIEN:    return "Plasma Gel";
                case PET_KAIJU:    return "Ration Slab";
                case PET_ANUBIS:   return "Sacred Grain";
                case PET_AXOLOTL:  return "Worm Treats";
                default:           return "Soul Food";
            }
        case ITEM_CURSED_RELIC:
            switch (petType) {
                case PET_DEVIL:    return "Cursed Relic";
                case PET_ELDRITCH: return "Void Trinket";
                case PET_ALIEN:    return "Signal Chip";
                case PET_KAIJU:    return "Shiny Stone";
                case PET_ANUBIS:   return "Scarab Charm";
                case PET_AXOLOTL:  return "Shroom Toy";
                default:           return "Cursed Relic";
            }
        case ITEM_DEMON_BONE:
            switch (petType) {
                case PET_DEVIL:    return "Demon Bone";
                case PET_ELDRITCH: return "Coral Shard";
                case PET_ALIEN:    return "Energy Cell";
                case PET_KAIJU:    return "Iron Girder";
                case PET_ANUBIS:   return "Ankh Piece";
                case PET_AXOLOTL:  return "Moss Pillow";
                default:           return "Demon Bone";
            }
        case ITEM_RITUAL_CHALK:
            switch (petType) {
                case PET_DEVIL:    return "Ritual Chalk";
                case PET_ELDRITCH: return "Ink Vial";
                case PET_ALIEN:    return "Med-Patch";
                case PET_KAIJU:    return "Scale Salve";
                case PET_ANUBIS:   return "Burial Wrap";
                case PET_AXOLOTL:  return "Gill Drops";
                default:           return "Ritual Chalk";
            }
        case ITEM_ELDRITCH_EYE:
            return "Eldritch Eye";
        default:
            return "???";
    }
}

const char* itemDesc(ItemType type) {
    switch (type) {
        case ITEM_SOUL_FOOD:    return "Restores hunger, +happiness, +energy";
        case ITEM_CURSED_RELIC: return "Boosts happiness significantly";
        case ITEM_DEMON_BONE:   return "Restores energy significantly";
        case ITEM_RITUAL_CHALK: return "Restores health significantly";
        case ITEM_ELDRITCH_EYE: return "Triggers evolution (if eligible)";
        default:                return "";
    }
}
