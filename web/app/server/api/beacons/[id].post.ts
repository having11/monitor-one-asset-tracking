import { BeaconDistDto } from "~/server/plugins/particleEvents";
import saveNewLocation from '../../utils/calcLocation';

export default defineEventHandler(async event => {
    const data = await readBody<BeaconDistDto>(event);

    await addBeaconDistanceEntry(data.source, data.beacon_minor, data.distance_m);
    await saveNewLocation(data.beacon_minor);

    return await getBeaconLocation(data.beacon_minor);
});
