export default defineEventHandler(async event => {
    const id = Number.parseInt(getRouterParam(event, 'id') ?? '0');

    return await getItemById(id);
});