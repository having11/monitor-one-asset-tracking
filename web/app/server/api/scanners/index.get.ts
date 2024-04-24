import { getAllScanners } from "~/server/utils/db";

export default defineEventHandler(async event => {
    return await getAllScanners();
});