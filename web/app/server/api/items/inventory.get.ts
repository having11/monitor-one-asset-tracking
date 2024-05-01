export default defineEventHandler(async event => {
  return await getCurrentInventoryForAllItems();
});