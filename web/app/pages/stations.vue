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
import type { Listener } from '~/models/Listener';

const { data, pending, error, refresh } = await useLazyAsyncData<Listener[]>('beacons',
    () => $fetch('/api/listeners', {
        method: 'get',
    })
);

const headers = [
    {
        title: 'ID',
        key: 'id',
        align: 'center',
    },
    {
        title: 'Latitude',
        key: 'latitude',
        align: 'center',
    },
    {
        title: 'Longitude',
        key: 'longitude',
        align: 'center',
    },
];
</script>