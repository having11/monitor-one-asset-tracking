import type { Coords } from "./Coords";

export type Beacon = {
    id: number;
    latestDistance?: number;
    location?: Coords;
};