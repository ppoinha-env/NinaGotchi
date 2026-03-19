#include "shop.h"
#include "inventory.h"
#include "pet.h"

const ShopItem shopCatalog[] = {
    { ITEM_SOUL_FOOD,     20  },
    { ITEM_CURSED_RELIC,  20  },
    { ITEM_DEMON_BONE,    20  },
    { ITEM_RITUAL_CHALK,  50  },
    { ITEM_ELDRITCH_EYE,  200 },
};

const int SHOP_ITEM_COUNT = (int)(sizeof(shopCatalog) / sizeof(shopCatalog[0]));

bool shopBuyItem(int catalogIndex) {
    if (catalogIndex < 0 || catalogIndex >= SHOP_ITEM_COUNT) return false;

    const ShopItem &item = shopCatalog[catalogIndex];

    // Check if player can afford it
    if (!spendInf(item.price)) return false;

    // Add to inventory
    if (!inventory.addItem(item.type, 1)) {
        // Refund if inventory full
        addInf(item.price);
        return false;
    }

    return true;
}

int shopGetPrice(int catalogIndex) {
    if (catalogIndex < 0 || catalogIndex >= SHOP_ITEM_COUNT) return 0;
    return shopCatalog[catalogIndex].price;
}
