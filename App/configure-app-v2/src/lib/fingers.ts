export type Hand = 'L' | 'R';
export type FingerName = 'thumb' | 'index' | 'middle' | 'ring' | 'pinky';
export type Finger = `${Hand}-${FingerName}`;

export const ALL_FINGERS: Finger[] = [
  'L-thumb',
  'L-index',
  'L-middle',
  'L-ring',
  'L-pinky',
  'R-thumb',
  'R-index',
  'R-middle',
  'R-ring',
  'R-pinky',
];

export const FINGERS_BY_HAND: Record<Hand, Finger[]> = {
  L: ALL_FINGERS.filter(f => f.startsWith('L-')),
  R: ALL_FINGERS.filter(f => f.startsWith('R-')),
};
