import { nanoid } from 'nanoid/non-secure';
import { create } from 'zustand';
import { persist } from 'zustand/middleware';

import { ALL_FINGERS, type Finger, type Hand } from './fingers';
import type { Waveform } from './waveform';

export type Mode = {
  id: string;
  name: string;
  enabled: boolean;
  color: string; // hex
  waveformId?: string;
  fingers: Set<Finger>;
  ui: { collapsed: boolean };
};

export type WaveformDoc = { id: string } & Waveform;

export type AppState = {
  modes: Mode[];
  fingerOwner: Record<Finger, string | null>; // modeId or null
  connected: boolean;
  deviceInfo?: { name?: string } | null;

  waveforms: WaveformDoc[];

  // actions
  addMode: (partial?: Partial<Mode>) => string;
  duplicateMode: (modeId: string) => string;
  removeMode: (modeId: string) => void;
  renameMode: (modeId: string, name: string) => void;
  toggleMode: (modeId: string, enabled?: boolean) => void;
  reorderModes: (from: number, to: number) => void;

  assignFinger: (modeId: string, finger: Finger) => void;
  unassignFinger: (modeId: string, finger: Finger) => void;
  selectAll: (modeId: string) => void;
  selectHand: (modeId: string, hand: Hand) => void;

  setWaveform: (modeId: string, waveformId?: string) => void;
  setColor: (modeId: string, hex: string) => void;

  collapseMode: (modeId: string, collapsed: boolean) => void;

  addWaveform: (wf: Waveform) => string;
  updateWaveform: (id: string, wf: Partial<Waveform>) => void;
  removeWaveform: (id: string) => void;

  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  send: () => Promise<void>;
};

const createEmptyOwner = (): Record<Finger, string | null> =>
  Object.fromEntries(ALL_FINGERS.map(f => [f, null])) as Record<Finger, string | null>;

const createMode = (partial?: Partial<Mode>): Mode => ({
  id: nanoid(6),
  name: 'Mode',
  enabled: true,
  color: '#60a5fa',
  fingers: new Set(),
  ui: { collapsed: false },
  ...partial,
});

export const useAppStore = create<AppState>()(
  persist(
    (set, get) => ({
      modes: [createMode({ name: 'Mode 1' })],
      fingerOwner: createEmptyOwner(),
      connected: false,
      deviceInfo: null,

      waveforms: [
        {
          id: nanoid(6),
          name: 'Pulse',
          totalTicks: 20,
          changeAt: [
            { tick: 0, output: 'high' },
            { tick: 10, output: 'low' },
          ],
        },
      ],

      addMode: partial => {
        const mode = createMode({ name: `Mode ${get().modes.length + 1}`, ...partial });
        set(s => ({ modes: [...s.modes, mode] }));
        return mode.id;
      },
      duplicateMode: modeId => {
        const src = get().modes.find(m => m.id === modeId);
        if (!src) return '';
        const copy = createMode({
          name: `${src.name} Copy`,
          enabled: src.enabled,
          color: src.color,
          waveformId: src.waveformId,
        });
        // Fingers are not copied by default to avoid conflicts
        set(s => ({ modes: [...s.modes, copy] }));
        return copy.id;
      },
      removeMode: modeId => {
        set(s => ({
          modes: s.modes.filter(m => m.id !== modeId),
          fingerOwner: Object.fromEntries(
            Object.entries(s.fingerOwner).map(([f, owner]) => [f, owner === modeId ? null : owner])
          ) as Record<Finger, string | null>,
        }));
      },
      renameMode: (modeId, name) => set(s => ({
        modes: s.modes.map(m => (m.id === modeId ? { ...m, name } : m)),
      })),
      toggleMode: (modeId, enabled) => set(s => ({
        modes: s.modes.map(m => (m.id === modeId ? { ...m, enabled: enabled ?? !m.enabled } : m)),
      })),
      reorderModes: (from, to) => set(s => {
        const arr = [...s.modes];
        const [item] = arr.splice(from, 1);
        arr.splice(to, 0, item);
        return { modes: arr };
      }),

      assignFinger: (modeId, finger) => set(s => {
        const nextOwner = { ...s.fingerOwner };
        // Unassign from previous owner if any
        if (nextOwner[finger] && nextOwner[finger] !== modeId) {
          nextOwner[finger] = null;
        }
        nextOwner[finger] = modeId;
        return { fingerOwner: nextOwner };
      }),
      unassignFinger: (modeId, finger) => set(s => {
        if (s.fingerOwner[finger] !== modeId) return { fingerOwner: s.fingerOwner };
        return { fingerOwner: { ...s.fingerOwner, [finger]: null } };
      }),
      selectAll: modeId => set(s => {
        const next = { ...s.fingerOwner };
        for (const f of ALL_FINGERS) next[f] = modeId;
        return { fingerOwner: next };
      }),
      selectHand: (modeId, hand) => set(s => {
        const next = { ...s.fingerOwner };
        for (const f of ALL_FINGERS) if (f.startsWith(hand)) next[f] = modeId;
        return { fingerOwner: next };
      }),

      setWaveform: (modeId, waveformId) => set(s => ({
        modes: s.modes.map(m => (m.id === modeId ? { ...m, waveformId } : m)),
      })),
      setColor: (modeId, hex) => set(s => ({
        modes: s.modes.map(m => (m.id === modeId ? { ...m, color: hex } : m)),
      })),

      collapseMode: (modeId, collapsed) => set(s => ({
        modes: s.modes.map(m => (m.id === modeId ? { ...m, ui: { ...m.ui, collapsed } } : m)),
      })),

      addWaveform: wf => {
        const id = nanoid(6);
        set(s => ({ waveforms: [...s.waveforms, { id, ...wf }] }));
        return id;
      },
      updateWaveform: (id, wf) => set(s => ({
        waveforms: s.waveforms.map(x => (x.id === id ? { ...x, ...wf } : x)),
      })),
      removeWaveform: id => set(s => ({
        waveforms: s.waveforms.filter(x => x.id !== id),
        modes: s.modes.map(m => (m.waveformId === id ? { ...m, waveformId: undefined } : m)),
      })),

      connect: async () => {
        // Mock client
        set({ connected: true, deviceInfo: { name: 'Mock Device' } });
      },
      disconnect: async () => {
        set({ connected: false, deviceInfo: null });
      },
      send: async () => {
        const s = get();
        const payload = s.modes.map(m => ({
          id: m.id,
          name: m.name,
          enabled: m.enabled,
          color: m.color,
          waveformId: m.waveformId,
          fingers: ALL_FINGERS.filter(f => s.fingerOwner[f] === m.id),
        }));
        console.log('SEND', payload);
      },
    }),
    { name: 'bulbchips-store' }
  )
);
