import { Item } from "~/models/Item";
import { createItem } from "~/server/utils/db";

export default defineEventHandler(async event => {
    const newItem = await readBody<Item>(event);

    return await createItem(newItem);
});