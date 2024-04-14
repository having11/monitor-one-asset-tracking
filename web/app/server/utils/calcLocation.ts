import { Beacon } from "~/models/Beacon";
import { BeaconDistanceEntry, getLatestBeaconDistance, updateBeaconLocation } from "./db";
import { Coords } from "~/models/Coords";

export default async function saveNewLocation(beaconId: number): Promise<Beacon | undefined> {
  const listenerLocations = await getListeners();
  const latestDistances = (await Promise.all(listenerLocations.map(l => getLatestBeaconDistance(l.id, beaconId))))
    .filter(d => d !== undefined)
    .sort((a, b) => (a?.ts ?? -Infinity) - (b?.ts ?? -Infinity));

  // Unable to triangulate
  if (!latestDistances || latestDistances.length < 3) {
    return undefined;
  }

  // const loc = await triangulate(<BeaconDistanceEntry[]>latestDistances);
  const loc = await trilaterate(<BeaconDistanceEntry[]>latestDistances);
  console.log(loc);

  if (!Number.isNaN(loc.latitude) && !Number.isNaN(loc.longitude)) {
    await updateBeaconLocation(beaconId, loc);

    return {
      id: beaconId,
      latestDistance: latestDistances[-1]?.distance,
      location: loc,
    };
  }

  return undefined;
};

type ListenerDistance = {
  latitude: number;
  longitude: number;
  distance: number;
  listenerId: number;
};

// See https://pastebin.com/9Dur6RAP

const DEG2RADs = (val: number) => (val * Math.PI) / 180;
const RAD2DEGs = (val: number) => (val * 180) / Math.PI;

async function trilaterate(entries: BeaconDistanceEntry[]): Promise<Coords> {
  const topEntries = entries.slice(-3);
  const ld: ListenerDistance[] = [];

  for (const entry of topEntries) {
    const listener = await getListener(entry.listenerId);
    if (!listener) {
      throw new Error(`Unable to find listener ${listener}`);
    }

    ld.push({
      latitude: listener.latitude,
      longitude: listener.longitude,
      distance: entry.distance,
      listenerId: listener.id,
    });
  }

    //Constants
    const earthR: number = 6371;
    const LatA: number = ld[0].latitude;
    const LonA: number = ld[0].longitude;
    const DistA: number = ld[0].distance / 1000;
    const LatB: number = ld[1].latitude;
    const LonB: number = ld[1].longitude;
    const DistB: number = ld[1].distance / 1000;
    const LatC: number = ld[2].latitude;
    const LonC: number = ld[2].longitude;
    const DistC: number = ld[2].distance / 1000;
    
    //uMath.sing authalic sphere
    //if uMath.sing an ellipsoid this step is slightly different
    //Convert geodetic Lat/Long to ECEF xyz
    //   1. Convert Lat/Long to radians
    //   2. Convert Lat/Long(radians) to ECEF
    const xA = earthR *(Math.cos(DEG2RADs(LatA)) * Math.cos(DEG2RADs(LonA)));
    const yA = earthR *(Math.cos(DEG2RADs(LatA)) * Math.sin(DEG2RADs(LonA)));
    const zA = earthR *(Math.sin(DEG2RADs(LatA)));
    
    const xB = earthR *(Math.cos(DEG2RADs(LatB)) * Math.cos(DEG2RADs(LonB)));
    const yB = earthR *(Math.cos(DEG2RADs(LatB)) * Math.sin(DEG2RADs(LonB)));
    const zB = earthR *(Math.sin(DEG2RADs(LatB)));
    
    const xC = earthR *(Math.cos(DEG2RADs(LatC)) * Math.cos(DEG2RADs(LonC)));
    const yC = earthR *(Math.cos(DEG2RADs(LatC)) * Math.sin(DEG2RADs(LonC)));
    const zC = earthR *(Math.sin(DEG2RADs(LatC)));
    
    const P1 = new Array<number>(xA,yA,zA);
    const P2 = new Array<number>(xB, yB, zB);
    const P3 = new Array<number>(xC, yC, zC);
    
    //ex = (P2 - P1)/(numpy.linalg.norm(P2 - P1))
    //i = dot(ex, P3 - P1)
    //ey = (P3 - P1 - i*ex)/(numpy.linalg.norm(P3 - P1 - i*ex))
    //ez = numpy.cross(ex,ey)
    //d = numpy.linalg.norm(P2 - P1)
    //j = dot(ey, P3 - P1)
    const ex = vectorDivScalar( vectorDiff(P2, P1) , vectorNorm(vectorDiff(P2, P1)));
    const i = vectorDot(ex, vectorDiff(P3, P1)); //PROB
    const diff = vectorDiff(vectorDiff(P3, P1), vectorMulScalar(i, ex));
    const ey = vectorDivScalar(diff, vectorNorm(diff));
    const ez = vectorCross(ex, ey);
    const d = vectorNorm( vectorDiff(P2, P1) );
    const j = vectorDot(ey, vectorDiff(P3, P1));
    
    //from wikipedia
    //plug and chug uMath.sing above values
    const x = (Math.pow(DistA,2) - Math.pow(DistB,2) + Math.pow(d,2))/(2*d);
    const y = ((Math.pow(DistA,2) - Math.pow(DistC,2) + Math.pow(i,2) + Math.pow(j,2))/(2*j)) - ((i/j)*x);
    
    //only one case shown here
    const z = Math.sqrt(Math.pow(DistA,2) - Math.pow(x,2) - Math.pow(y,2));
    
    //triPt is an array with ECEF x,y,z of trilateration point
    //triPt = P1 + x*ex + y*ey + z*ez
    const triPt = vectorAdd(vectorAdd(P1, vectorMulScalar(x, ex)), vectorAdd(vectorMulScalar(y, ey), vectorMulScalar(z, ez)));
    
    //convert back to lat/long from ECEF
    //convert to degrees
    const lat = RAD2DEGs(Math.asin(triPt[2] / earthR));
    const lon = RAD2DEGs(Math.atan2(triPt[1],triPt[0]));
    
    return {
        latitude: lat,
        longitude: lon
    };
}

function vectorMul(a: number[], b: number[]): number[] {
    let result = new Array<number>(0,0,0);
    result[0]=a[0]*b[0]; result[1]=a[1]*b[1]; result[2]=a[2]*b[2];
    return result;
}

function vectorMulScalar(a: number, b: number[]) {
    let result = new Array<number>(0,0,0);
    result[0]=a*b[0]; result[1]=a*b[1]; result[2]=a*b[2];
    return result;
}

function vectorDiv(a: number[], b: number[]){
    let result = new Array<number>(0,0,0);
    result[0]=a[0]/b[0]; result[1]=a[1]/b[1]; result[2]=a[2]/b[2];
    return result;
}

function vectorDivScalar(a: number[], b: number){
    let result = new Array<number>(0,0,0);
    result[0]=a[0]/b; result[1]=a[1]/b; result[2]=a[2]/b;
    return result;
}

function vectorDiff(a: number[], b: number[]) {
    let result = new Array<number>(0,0,0);
    result[0]=a[0]-b[0]; result[1]=a[1]-b[1]; result[2]=a[2]-b[2];
    return result;
}

function vectorAdd(a: number[],b: number[]) {
    let result = new Array<number>(0,0,0);
    result[0]=a[0]+b[0]; result[1]=a[1]+b[1]; result[2]=a[2]+b[2];
    return result;
}

function vectorDot(a: number[],b: number[]): number {
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

function vectorCross(a: number[], b: number[]){
    let result = new Array<number>(0,0,0);
    result[0] = a[1]*b[2]-a[2]*b[1];
    result[1] = a[2]*b[0]-a[0]*b[2];
    result[2] = a[0]*b[1]-a[1]*b[0];
    return result;
}

function vectorNorm(a: number[]): number {
    return Math.sqrt(vectorDot(a, a));
}

function vectorNormalize(a: number[]){
    let result = new Array<number>(0,0,0);
    const mag = Math.sqrt(vectorDot(a,a));
    result[0] = a[0]/mag;
    result[1] = a[1]/mag;
    result[2] = a[2]/mag;
    return result;
}