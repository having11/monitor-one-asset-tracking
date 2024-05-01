export type schema_item = {
    id: number;
    name: string;
    description: string | null;
};

export type schema_scanner = {
    id: number;
    name: string;
    description: string | null;
    latitude: number;
    longitude: number;
};

export type schema_inventory_event = {
    id: number;
    timestamp: Date;
    quantity_change: number;
    item_id: number;
    scanner_id: number;
};

export type schema_vw_item_inventory = {
    item_id: number;
    current_quantity: number;
    scan_count: number;
    item_name: string;
};

export type schema_vw_latest_scan = {
    item_id: number;
    timestamp: Date;
    quantity_change: number;
    scanner_id: number;
    item_name: string;
    scanner_name: string;
    scanner_latitude: number;
    scanner_longitude: number;
};