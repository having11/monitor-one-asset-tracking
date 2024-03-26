<script setup lang="ts">
import Map from '~/components/Map.vue';

const route = useRoute();
</script>

<template>
  <div>
    <h1>Nuxt Routing set up successfully!</h1>
    <p>Current route: {{ route.path }}</p>
    <Map />
  </div>
</template>
