import type { Waveform } from './waveform';

export const DEFAULT_NEW_WAVEFORM: Waveform = {
  name: 'New Pattern',
  totalTicks: 16,
  changeAt: [{ tick: 0, output: 'high' }],
};

// Five varied default waveforms. totalTicks kept small/consistent for easy previewing.
// All waveforms satisfy zWaveform constraints: first change at tick 0 and strictly increasing.
export const DEFAULT_WAVEFORMS: Waveform[] = [
  // 1) Classic 50% duty pulse (baseline)
  {
    name: 'Pulse',
    totalTicks: 20,
    changeAt: [
      { tick: 0, output: 'high' },
      { tick: 10, output: 'low' },
    ],
  },
  // 2) Short blink (brief on, long off)
  {
    name: 'Blink (short)',
    totalTicks: 20,
    changeAt: [
      { tick: 0, output: 'high' },
      { tick: 3, output: 'low' },
    ],
  },
  // 3) Long on, short off
  {
    name: 'Beacon (long on)',
    totalTicks: 20,
    changeAt: [
      { tick: 0, output: 'high' },
      { tick: 16, output: 'low' },
    ],
  },
  // 4) Double blink within one cycle
  {
    name: 'Double Blink',
    totalTicks: 20,
    changeAt: [
      { tick: 0, output: 'high' },
      { tick: 3, output: 'low' },
      { tick: 6, output: 'high' },
      { tick: 9, output: 'low' },
    ],
  },
  // 5) Triple burst
  {
    name: 'Triple Burst',
    totalTicks: 20,
    changeAt: [
      { tick: 0, output: 'high' },
      { tick: 2, output: 'low' },
      { tick: 4, output: 'high' },
      { tick: 6, output: 'low' },
      { tick: 8, output: 'high' },
      { tick: 10, output: 'low' },
    ],
  },
];
