import type { schema_inventory_event } from "./db_schema";

export type InventoryEvent = {
    id: number;
    timestamp: Date;
    quantityChange: number;
    itemId: number;
    scannerId: number;
};

export const inventoryEventMap = (inventory_event: schema_inventory_event): InventoryEvent => {
    return {
        id: inventory_event.id,
        timestamp: inventory_event.timestamp,
        quantityChange: inventory_event.quantity_change,
        itemId: inventory_event.item_id,
        scannerId: inventory_event.scanner_id,
    };
};
