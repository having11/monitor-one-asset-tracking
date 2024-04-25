<template>
  <v-dialog
    v-model="model.open"
    width="auto"
  >
    <v-card
      width="600"
      prepend-icon="mdi-shape"
      :title="`${props.itemName} details`"
    >
      <template v-slot:default>
        <div class="mb-2">
          <v-data-table class="mb-2"
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
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
export type ItemScanDialogModel = {
  open: boolean;
};

export type ItemScanDialogProps = {
  itemId: number;
  itemName: string;
};

import type { InventoryEvent } from '~/models/InventoryEvent';

const model = defineModel<ItemScanDialogModel>();
const props = defineProps<ItemScanDialogProps>();

const { data, pending, error, refresh } = await useLazyAsyncData<InventoryEvent[]>('itemScans',
    () => $fetch(`/api/items/${props.itemId}/scans`, {
        method: 'get',
    })
);

const headers = [
    {
        title: 'ID',
        key: 'id',
    },
    {
        title: 'Timestamp',
        key: 'timestamp',
        value: (item: any) => new Date(item.timestamp).toLocaleString(),
    },
    {
        title: 'Quantity Change',
        key: 'quantityChange',
    },
    {
        title: 'Scanner ID',
        key: 'scannerId',
    },
];
</script>