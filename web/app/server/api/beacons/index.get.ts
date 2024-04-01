import { getAllBeaconIds, getBeaconLocation, getListener } from "~/server/utils/db";

export default defineEventHandler(async event => {
    const beaconIds = await getAllBeaconIds();

    return await Promise.all(beaconIds.map(async id => await getBeaconLocation(id)));
});