import { getAllItems } from "~/server/utils/db";

export default defineEventHandler(async event => {
  return await getAllItems();
});