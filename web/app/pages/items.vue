<template>
    <div>
        <v-data-table
            :items="pending ? [] : data"
            :loading="pending"
            loading-text="Loading..."
            :headers="headers"
            :items-length="data?.length ?? 0"
            :items-per-page="10"
            @update:options="refresh">
            <template v-slot:item.itemId="{ value, item }">
                <v-btn append-icon="mdi-open-in-new" variant="text" @click="() => openDialog(item)">
                    {{ value }}
                </v-btn>
            </template>
        </v-data-table>
    </div>
    <ItemScanDialog
      v-model="dialogOpen"
      :itemId="itemScansData.itemId"
      :itemName="itemScansData.itemName"
    />
</template>

<script setup lang="ts">
import { reactive, ref } from "vue";
import type { ItemInventory } from "~/models/ItemInventory";

const { data, pending, error, refresh } = await useLazyAsyncData<ItemInventory[]>('itemInventoryList',
    () => $fetch('/api/items/inventory', {
        method: 'get',
    })
);

const headers = [
    {
        title: 'Item ID',
        key: 'itemId',
        align: 'center',
    },
    {
        title: 'Item Name',
        key: 'itemName',
    },
    {
        title: 'Quantity',
        key: 'currentQuantity',
    },
    {
        title: 'Scan Count',
        key: 'scanCount',
    },
];

const dialogOpen = reactive({ open: false });
const itemScansData = reactive({ itemId: 0, itemName: "" });

function openDialog(item: any) {
  itemScansData.itemId = item.itemId;
  itemScansData.itemName = item.itemName;
  dialogOpen.open = true;
}
</script>