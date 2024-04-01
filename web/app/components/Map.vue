<template>
    <div style="height:75vh; width:80vw">
    <LMap
      ref="map"
      :zoom="zoom"
      :center="[47.21322, -1.559482]"
    >
      <LTileLayer
        url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
        layer-type="base"
        name="OpenStreetMap"
      />
    </LMap>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
const zoom = ref(6);
</script>

<style>
body {
  margin: 0;
}
</style>
