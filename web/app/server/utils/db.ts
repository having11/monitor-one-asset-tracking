import type { Listener } from "~/models/Listener";
import { createClient } from "redis";
import pg from 'pg';
import type { Coords } from "~/models/Coords";
import type { Beacon } from "~/models/Beacon";

export type BeaconDistanceEntry = {
  listenerId: number;
  ts: number;
  distance: number;
};

const postgresClient = new pg.Client({
    user: process.env.PG_USERNAME,
    host: process.env.PG_HOST,
    database: process.env.PG_DATABASE,
    password: process.env.PG_PASSWORD,
    port: Number.parseInt(process.env.PG_PORT || '5432'),
    });
await postgresClient.connect();
    
const redisClient = createClient({
    url: process.env.REDIS_CONN_STRING
    });
    redisClient.on('error', err => console.log('Redis client error', err));
await redisClient.connect();

export async function getListeners() {
  const members = await redisClient.SMEMBERS('listeners');
  
  const listeners: Listener[] = [];
  for await (const entry of members) {
    const listenerKey = `listener:${entry}`;
    const listener = await getListenerKey(listenerKey);
    if (listener) listeners.push(listener);
  }

  return listeners;
};

async function addListener(id: number) {
  await redisClient.SADD('listeners', id.toString());
}

export async function getListener(id: number): Promise<Listener | undefined> {
  const listenerKey = `listener:${id}`;
  return await getListenerKey(listenerKey);
};

async function getListenerKey(key: string): Promise<Listener | undefined> {
  const keyExists = await redisClient.EXISTS(key);
  if (keyExists === 0) {
    return undefined;
  }

  const listener = await redisClient.HGETALL(key);
  return {
    id: Number.parseInt(listener['id']),
    latitude: Number.parseFloat(listener['latitude']),
    longitude: Number.parseFloat(listener['longitude']),
  };
};

export async function createListener(body: Listener) {
  const listenerKey = `listener:${body.id}`;
  await addListener(body.id);
  await redisClient.HSET(listenerKey, body);
};

export async function updateListener(id: number, body: Listener): Promise<Listener | undefined> {
  const listenerKey = `listener:${id}`;
  const existingListener = await getListenerKey(listenerKey);

  if (!existingListener) {
    return undefined;
  }

  await redisClient.HSET(listenerKey, body);

  return await getListenerKey(listenerKey);
};

export async function getAllBeaconIds() {
  return (await redisClient.SMEMBERS('beacons')).map(b => Number.parseInt(b));
}

async function addBeacon(beaconId: number) {
  await redisClient.SADD('beacons', beaconId.toString());
}

export async function addBeaconDistanceEntry(listenerId: number, beaconId: number, distanceMeters: number) {
  const listener = await getListener(listenerId);
  if (!listener) {
    throw new Error(`Unable to find listener ${listenerId}`);
  }

  await addBeacon(beaconId);

  const entryKey = `distance:beacon:${beaconId}:${listenerId}`;

  await redisClient.ts.ADD(entryKey, '*', distanceMeters);
}

export async function getLatestBeaconDistance(listenerId: number, beaconId: number): Promise<BeaconDistanceEntry | undefined> {
  const listener = await getListener(listenerId);
  if (!listener) {
    throw new Error(`Unable to find listener ${listenerId}`);
  }

  const entryKey = `distance:beacon:${beaconId}:${listenerId}`;
  
  if ((await redisClient.EXISTS(entryKey)) === 0) {
    return undefined;
  }

  const entry = await redisClient.ts.GET(entryKey);
  if (!entry) {
    return undefined;
  }

  return {
    listenerId,
    ts: entry.timestamp,
    distance: entry.value
  };
}

export async function getBeaconLocation(id: number): Promise<Beacon | undefined> {
  const beaconKey = `beacons:${id}`;

  const beaconLocations = await redisClient.GEOPOS('locations', beaconKey);

  if (!beaconLocations || beaconLocations.length !== 1) {
    return undefined;
  }

  const location = beaconLocations[0];
  if (!location) {
    return undefined;
  }

  return {
    id: id,
    location: {
      latitude: Number.parseFloat(location.latitude),
      longitude: Number.parseFloat(location.longitude)
    }
  }
}

export async function updateBeaconLocation(id: number, location: Coords): Promise<Beacon> {
  const beaconKey = `beacons:${id}`;

  await redisClient.GEOADD('locations', {
    longitude: location.longitude,
    latitude: location.latitude,
    member: beaconKey,
  });

  const updatedLocation = await getBeaconLocation(id);

  if (!updatedLocation) {
    throw new Error(`Error updating location for beacon ${id}`);
  }

  return updatedLocation;
}