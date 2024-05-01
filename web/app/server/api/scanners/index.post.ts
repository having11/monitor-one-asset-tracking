import { Scanner } from "~/models/Scanner";
import { createScanner } from "~/server/utils/db";

export default defineEventHandler(async event => {
    const newScanner = await readBody<Scanner>(event);

    return await createScanner(newScanner);
});