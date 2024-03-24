import { Listener } from "~/models/Listener";
import { createListener, getListener, getListeners } from "~/server/utils/db";

export default defineEventHandler(async event => {
  const newListener = await readBody<Listener>(event);
  const listener = await getListener(newListener.id);

  if (listener) {
    throw createError({
        statusCode: 401,
        statusMessage: `Listener with ID ${newListener.id} already exists`
    });
  }

  await createListener(newListener);
});