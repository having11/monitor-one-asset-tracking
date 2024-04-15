import saveNewLocation from '../../utils/calcLocation';

export default defineEventHandler(async event => {
    const id = Number.parseInt(getRouterParam(event, 'id') ?? '0');
    await saveNewLocation(id);

    return await getBeaconLocation(id);
});
