import { InventoryEvent } from "~/models/InventoryEvent";

export default defineEventHandler(async event => {
    const id = Number.parseInt(getRouterParam(event, 'id') ?? '0');
    const newScan = await readBody<InventoryEvent>(event);

    return await createItemScan(id, newScan);
});