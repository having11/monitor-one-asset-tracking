import Particle from 'particle:core';

export default function reformat({ event }) {
  let data;
  try {
	  data = JSON.parse(event.eventData);
  } catch (err) {
    console.error("Invalid JSON", event.eventData);
    throw err;
  }
  const reformatted = {
    id: 0,
    timestamp: new Date().toISOString(),
    quantityChange: data.quantity,
    itemId: data.tagId,
    scannerId: data.scannerId,
  };
  Particle.publish("INVENTORY-SCAN", reformatted, { productId: 22124 });
}