import type { schema_vw_item_inventory } from "./db_schema";

export type ItemInventory = {
    itemId: number;
    itemName: string;
    currentQuantity: number;
    scanCount: number;
};

export const itemInventoryMap = (item_inventory: schema_vw_item_inventory): ItemInventory => {
    return {
        itemId: item_inventory.item_id,
        itemName: item_inventory.item_name,
        currentQuantity: item_inventory.current_quantity,
        scanCount: item_inventory.scan_count,
    };
};
