<template>
<v-card>
    <v-navigation-drawer
        v-model="drawer"
        :rail="rail"
        permanent
        @click="rail = false"
        id="nav-drawer-main"
    >
        <v-list-item nav>
        <template v-slot:append>
            <v-btn
            :icon="`${rail ? 'mdi-chevron-right' : 'mdi-chevron-left'}`"
            variant="text"
            @click.stop="rail = !rail"
            ></v-btn>
        </template>
        </v-list-item>

        <v-divider></v-divider>

        <v-list density="default" nav>
        <v-list-item prepend-icon="mdi-map-outline" title="View Map" value="map" :to="'/'"></v-list-item>
        <v-divider></v-divider>
        <v-list-item prepend-icon="mdi-access-point" title="Beacons" value="beacons" :to="'/beacons'">
        </v-list-item>
        <v-divider></v-divider>
        <v-list-item prepend-icon="mdi-satellite-uplink" title="Stations" value="stations" :to="'/stations'">
        </v-list-item>
        <v-divider></v-divider>
        <v-list-item prepend-icon="mdi-view-list-outline" title="Events" value="logs" :to="'/events'">
        </v-list-item>
        </v-list>
    </v-navigation-drawer>
</v-card>
</template>

<script>
export default {
  data () {
    return {
      drawer: true,
      rail: true,
    };
  },
}
</script>