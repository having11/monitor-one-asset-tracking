import EventSource from 'eventsource';
import saveNewLocation from '../utils/calcLocation';

export type BeaconDistDto = {
    source: number;
    beacon_minor: number;
    distance_m: number;
};

export default defineNitroPlugin(nitroApp => {
    const url = `https://api.particle.io/v1/events/BEACON?access_token=${process.env.PARTICLE_API_TOKEN}`;

    const events = new EventSource(url);

    events.onerror = err => {
        console.log('Got error', err);
    }

    events.addEventListener('BEACON-DIST', async evt => {
        console.log('Got Particle event', evt);
        const event = JSON.parse(evt.data);
        const dto = event.data as BeaconDistDto;

        await addBeaconDistanceEntry(dto.source, dto.beacon_minor, dto.distance_m);
        await saveNewLocation(dto.beacon_minor);
    });

    events.onopen = evt => {
        console.log('opened connection');
    }
});