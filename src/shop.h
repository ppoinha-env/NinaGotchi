#pragma once
#include "game_types.h"

struct ShopItem {
    ItemType type;
    int      price;
};

extern const ShopItem shopCatalog[];
extern const int SHOP_ITEM_COUNT;

// Buy an item from shop. Deducts INF, adds to inventory.
// Returns true on success.
bool shopBuyItem(int catalogIndex);

// Get the price of a catalog item
int shopGetPrice(int catalogIndex);
