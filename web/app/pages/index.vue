<script setup lang="ts">
import Map from '~/components/Map.vue';

const route = useRoute();
</script>

<template>
  <div id="map-container">
    <h2>Map</h2>
    <Map />
  </div>
</template>

<style scoped>
#map-container {
    margin-top: 20px;
}
</style>