import { getListeners } from "~/server/utils/db";

export default defineEventHandler(async event => {
  return await getListeners();
});