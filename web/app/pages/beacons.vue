<template>
    <div class="text-center">
        <v-data-table-server
            :items="pending ? [] : data"
            :loading="pending"
            loading-text="Loading..."
            :headers="headers"
            :items-length="data?.length ?? 0"
            :items-per-page="10"
            @update:options="refresh">
        </v-data-table-server>
    </div>
</template>

<script setup lang="ts">
import type { Beacon } from '~/models/Beacon';
import type { Coords } from '~/models/Coords';

const { data, pending, error, refresh } = await useLazyAsyncData<Beacon[]>('beacons',
    () => $fetch('/api/beacons', {
        method: 'get',
    })
);

const headers = [
    {
        title: 'ID',
        align: 'start',
        key: 'id'
    },
    {
        title: 'Latest distance',
        align: 'start',
        key: 'latestDistance',
        value: (item: number) => `${item}m`
    },
    {
        title: 'Location',
        align: 'start',
        key: 'location',
        value: (item: Coords) => `${item.latitude}, ${item.longitude}`,
    },
];
</script>