<template>
    <div class="text-center">
        <v-data-table
            :items="pending ? [] : data"
            :loading="pending"
            loading-text="Loading..."
            :headers="headers"
            :items-length="data?.length ?? 0"
            :items-per-page="10"
            @update:options="refresh">
        </v-data-table>
    </div>
</template>

<script setup lang="ts">
import type { Scanner } from '~/models/Scanner';

const { data, pending, error, refresh } = await useLazyAsyncData<Scanner[]>('scanners',
    () => $fetch('/api/scanners', {
        method: 'get',
    })
);

const headers = [
    {
        title: 'ID',
        key: 'id',
    },
    {
        title: 'Name',
        key: 'name',
    },
    {
        title: 'Description',
        key: 'description',
    },
    {
        title: 'Latitude',
        key: 'latitude',
    },
    {
        title: 'Longitude',
        key: 'longitude',
    },
];
</script>