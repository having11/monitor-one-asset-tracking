import { Beacon } from "~/models/Beacon";
import { BeaconDistanceEntry, getLatestBeaconDistance, updateBeaconLocation } from "./db";
import { Coords } from "~/models/Coords";

export default async function saveNewLocation(beaconId: number): Promise<Beacon | undefined> {
  const listenerLocations = await getListeners();
  const latestDistances = (await Promise.all(listenerLocations.map(l => getLatestBeaconDistance(l.id, beaconId))))
    .filter(d => d !== undefined)
    .sort((a, b) => (a?.ts ?? -Infinity) - (b?.ts ?? -Infinity));

  // Unable to triangulate
  if (!latestDistances || latestDistances.length < 3) {
    return undefined;
  }

  const loc = await triangulate(<BeaconDistanceEntry[]>latestDistances);

  await updateBeaconLocation(beaconId, loc);

  return {
    id: beaconId,
    latestDistance: latestDistances[-1]?.distance,
    location: loc,
  };
};

type ListenerDistance = {
  latitude: number;
  longitude: number;
  distance: number;
  listenerId: number;
};

// See https://stackoverflow.com/questions/20332856/triangulate-example-for-ibeacons
async function triangulate(entries: BeaconDistanceEntry[]): Promise<Coords> {
  const topEntries = entries.slice(-3);
  const ld: ListenerDistance[] = [];

  for (const entry of topEntries) {
    const listener = await getListener(entry.listenerId);
    if (!listener) {
      throw new Error(`Unable to find listener ${listener}`);
    }

    ld.push({
      latitude: listener.latitude,
      longitude: listener.longitude,
      distance: entry.distance,
      listenerId: listener.id,
    });
  }

  const w = ld[0].distance * ld[0].distance - ld[1].distance * ld[1].distance -
    ld[0].latitude * ld[0].latitude - ld[0].longitude * ld[0].longitude +
    ld[1].latitude * ld[1].latitude - ld[1].longitude * ld[1].longitude;

  const z = ld[1].distance * ld[1].distance - ld[2].distance * ld[2].distance -
    ld[1].latitude * ld[1].latitude - ld[1].longitude * ld[1].longitude +
    ld[2].latitude * ld[2].latitude - ld[2].longitude * ld[2].longitude;

  let beaconLat: number, beaconLong: number, beaconFilter: number;

  beaconLat = (w * (ld[2].longitude - ld[1].longitude) - z * (ld[1].longitude - ld[0].longitude)) /
    (2 * ((ld[1].latitude - ld[0].latitude) * (ld[2].longitude - ld[1].longitude) -
    (ld[2].latitude - ld[1].latitude) * (ld[1].longitude - ld[0].longitude)));
  beaconLong = (w - 2 * beaconLat * (ld[1].latitude - ld[0].latitude)) / (2 * (ld[1].longitude - ld[0].longitude));
  beaconFilter = (z - 2 * beaconLat * (ld[2].latitude - ld[1].latitude)) / (2 * (ld[2].longitude - ld[1].longitude));

  beaconLong = (beaconLong + beaconFilter) / 2;

  return {
    latitude: beaconLat,
    longitude: beaconLong,
  };
}