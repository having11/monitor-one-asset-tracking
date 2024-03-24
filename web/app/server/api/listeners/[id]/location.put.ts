import { Listener } from "~/models/Listener";
import { getListener, updateListener } from "~/server/utils/db";

export default defineEventHandler(async event => {
    const id = Number.parseInt(getRouterParam(event, 'id') ?? '-1');
    const listener = await getListener(id);

    if (!listener) {
        throw createError({
            statusCode: 404,
            statusMessage: `Listener with ID ${id} not found`
        });
    }

    const newListener = await readBody<Listener>(event);
    return await updateListener(id, newListener);
});