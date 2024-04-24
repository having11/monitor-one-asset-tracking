import type { schema_vw_latest_scan } from "./db_schema";

export type LatestScan = {
    itemId: number;
    itemName: string;
    timestamp: Date;
    quantityChange: number;
    scannerId: number;
    scannerLatitude: number;
    scannerLongitude: number;
};

export const latestScanMap = (latest_scan: schema_vw_latest_scan): LatestScan => {
    return {
        itemId: latest_scan.item_id,
        itemName: latest_scan.item_name,
        timestamp: latest_scan.timestamp,
        quantityChange: latest_scan.quantity_change,
        scannerId: latest_scan.scanner_id,
        scannerLatitude: latest_scan.scanner_latitude,
        scannerLongitude: latest_scan.scanner_longitude,
    };
};
