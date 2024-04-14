<template>
  <div style="height:75vh; width:80vw">
  <LMap
    ref="map"
    :zoom="zoom"
    :center="centerPoint"
  >
    <LTileLayer
      url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
      layer-type="base"
      name="OpenStreetMap"
    />
    <LPolygon :lat-lngs="data && data.map(point => [point.latitude, point.longitude])"></LPolygon>
    <LMarker :key="point.id" v-for="point in data" :lat-lng="[point.latitude, point.longitude]">
      <LPopup>
        Station {{ point.id }}
      </LPopup>
    </LMarker>
    <LCircle v-if="beaconData" :key="beacon.id" v-for="beacon in beaconData" :lat-lng="beacon.location ? [beacon.location.latitude, beacon.location.longitude] : [0, 0]"
    color="red"
    fill-color="red"
    :fill-opacity="0.5"
    :radius=2
    >
      <LPopup>
        Beacon {{ beacon.id }}
      </LPopup>
    </LCircle> 
  </LMap>
  <v-container>
    <v-row>
        <v-col cols="4">
            <div>
                {{ centerPoint }}
            </div>
        </v-col>
        <v-spacer></v-spacer>
        <v-col cols="4">
            <v-btn append-icon="mdi-refresh" variant="outlined" @click="beaconRefresh()">
                Refresh
            </v-btn>
        </v-col>
    </v-row>
  </v-container>
</div>
</template>

<script setup lang="ts">
import { ref } from 'vue';

const zoom = ref(12);
const map = ref(null);
const centerPoint = ref([0, 0]);

const exampleBeacons = [
  {
    id: 1,
    latestDistance: 4,
    location: {
      latitude: 47.15,
      longitude: -1.5
    },
  },
  {
    id: 2,
    latestDistance: 4,
    location: {
      latitude: 47.137,
      longitude: -1.49
    },
  }
];

const beacons = ref(exampleBeacons);

import type { Listener } from '~/models/Listener';
import type { Beacon } from '~/models/Beacon';

const { data, pending, error, refresh } = await useLazyFetch<Listener[]>('/api/listeners');
const { data: beaconData, pending: beaconPending, error: beaconError, refresh: beaconRefresh } = await useLazyFetch<Beacon[]>('/api/beacons');

const getCentroid = (points: Listener[]): number[] => {
  let cXSum = 0;
  let cYSum = 0;

  for (let i = 0; i < points.length; i++) {
    cXSum += points[i].latitude;
    cYSum += points[i].longitude;
  }

  const cX = cXSum / points.length;
  const cY = cYSum / points.length;

  return [cX, cY];
};

watch(data, async (newData, oldData) => {
  if (newData) {
    centerPoint.value = getCentroid(newData);
  }
});

onMounted(() => {
    const interval = setInterval(function() {
   beaconRefresh();
 }, 3000);
});
</script>

<style>
body {
  margin: 0;
}
</style>
