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
        title: 'Location',
        align: 'end',
        key: 'location',
        value: (item: Beacon) => `${item.location?.latitude}, ${item.location?.longitude}`,
    },
];
</script>