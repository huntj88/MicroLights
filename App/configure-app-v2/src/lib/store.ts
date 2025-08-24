import { nanoid } from 'nanoid/non-secure';
import { create } from 'zustand';
import { persist } from 'zustand/middleware';

import { ALL_FINGERS, type Finger, type Hand } from './fingers';
import type { Waveform } from './waveform';

// Allowed accelerometer threshold values
const ALLOWED_THRESHOLDS = [2, 4, 8, 12, 16] as const;

export type Mode = {
  id: string;
  enabled: boolean; // TODO: remove, should not used
  color: string; // hex
  waveformId?: string;
  fingers: Set<Finger>;
  ui: { collapsed: boolean };
  accel?: {
    enabled: boolean; // TODO: remove, should not be used
    triggers: Array<{
      threshold: number;
      waveformId?: string;
    }>;
  };
};

export type WaveformDoc = { id: string } & Waveform;

// A saved ModeSet snapshot of modes and finger ownership
export type ModeSnapshot = Pick<Mode, 'enabled' | 'color' | 'waveformId' | 'accel'>;
export type ModeSet = {
  id: string;
  name: string;
  modes: ModeSnapshot[];
  // maps each finger to the index of the owning mode in `modes` (or null)
  fingerOwnerIndex: Record<Finger, number | null>;
};

export type AppState = {
  modes: Mode[];
  fingerOwner: Record<Finger, string | null>; // modeId or null
  connected: boolean;
  deviceInfo?: { name?: string } | null;

  waveforms: WaveformDoc[];
  modeSets: ModeSet[];

  // UI settings
  theme: 'system' | 'light' | 'dark';

  // actions
  addMode: (partial?: Partial<Mode>) => string;
  duplicateMode: (modeId: string) => string;
  removeMode: (modeId: string) => void;
  toggleMode: (modeId: string, enabled?: boolean) => void;
  reorderModes: (from: number, to: number) => void;

  // theme actions
  setThemePreference: (pref: 'system' | 'light' | 'dark') => void;

  assignFinger: (modeId: string, finger: Finger) => void;
  unassignFinger: (modeId: string, finger: Finger) => void;
  selectAll: (modeId: string) => void;
  selectHand: (modeId: string, hand: Hand) => void;

  setWaveform: (modeId: string, waveformId?: string) => void;
  setColor: (modeId: string, hex: string) => void;

  // accelerometer actions
  setAccelEnabled: (modeId: string, enabled: boolean) => void;
  addAccelTrigger: (modeId: string) => void; // max 2
  removeAccelTrigger: (modeId: string, index: number) => void;
  setAccelTriggerThreshold: (modeId: string, index: number, threshold: number) => void;
  setAccelTriggerWaveform: (modeId: string, index: number, waveformId?: string) => void;

  collapseMode: (modeId: string, collapsed: boolean) => void;

  addWaveform: (wf: Waveform) => string;
  updateWaveform: (id: string, wf: Partial<Waveform>) => void;
  removeWaveform: (id: string) => void;

  // mode set library actions
  newModeSetDraft: () => void; // reset current working set
  saveCurrentModeSet: (name: string) => string; // returns set id
  updateModeSet: (id: string, name?: string) => void; // overwrite existing set with current state
  loadModeSet: (id: string) => void;
  renameModeSet: (id: string, name: string) => void;
  removeModeSet: (id: string) => void;

  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  send: () => Promise<void>;
};

const createEmptyOwner = (): Record<Finger, string | null> =>
  Object.fromEntries(ALL_FINGERS.map(f => [f, null])) as Record<Finger, string | null>;

const createMode = (partial?: Partial<Mode>): Mode => ({
  id: nanoid(6),
  enabled: true,
  color: '#60a5fa',
  fingers: new Set(),
  ui: { collapsed: false },
  accel: { enabled: false, triggers: [] },
  ...partial,
});

export const useAppStore = create<AppState>()(
  persist(
    (set, get) => ({
      modes: [createMode()],
      fingerOwner: createEmptyOwner(),
      connected: false,
      deviceInfo: null,

      // default to system theme; AppShell computes actual dark/light
      theme: 'system',

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

      modeSets: [],

      addMode: partial => {
        const mode = createMode({ ...partial });
        set(s => ({ modes: [...s.modes, mode] }));
        return mode.id;
      },
      duplicateMode: modeId => {
        const src = get().modes.find(m => m.id === modeId);
        if (!src) return '';
        const copy = createMode({
          enabled: src.enabled,
          color: src.color,
          waveformId: src.waveformId,
          accel: src.accel ? { enabled: src.accel.enabled, triggers: src.accel.triggers.map(t => ({ ...t })) } : { enabled: false, triggers: [] },
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
      toggleMode: (modeId, enabled) => set(s => ({
        modes: s.modes.map(m => (m.id === modeId ? { ...m, enabled: enabled ?? !m.enabled } : m)),
      })),
      reorderModes: (from, to) => set(s => {
        const arr = [...s.modes];
        const [item] = arr.splice(from, 1);
        arr.splice(to, 0, item);
        return { modes: arr };
      }),

      // theme
      setThemePreference: (pref) => set(() => ({ theme: pref })),

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

      // accelerometer
      setAccelEnabled: (modeId, enabled) => set(s => ({
        modes: s.modes.map(m => (m.id === modeId ? { ...m, accel: { ...(m.accel ?? { enabled: false, triggers: [] }), enabled } } : m)),
      })),
      addAccelTrigger: (modeId) => set(s => ({
        modes: s.modes.map(m => {
          if (m.id !== modeId) return m;
          const acc = m.accel ?? { enabled: false, triggers: [] };
          if (acc.triggers.length >= 2) return m;
          const prev = acc.triggers[acc.triggers.length - 1];
          const nextAllowed = prev
            ? ALLOWED_THRESHOLDS.find(v => v > prev.threshold)
            : ALLOWED_THRESHOLDS[0];
          if (nextAllowed == null) return m; // no valid higher threshold available
          return { ...m, accel: { ...acc, triggers: [...acc.triggers, { threshold: nextAllowed, waveformId: undefined }] } };
        })
      })),
      removeAccelTrigger: (modeId, index) => set(s => ({
        modes: s.modes.map(m => {
          if (m.id !== modeId) return m;
          const acc = m.accel ?? { enabled: false, triggers: [] };
          const next = acc.triggers.filter((_, i) => i !== index);
          return { ...m, accel: { ...acc, triggers: next } };
        })
      })),
      setAccelTriggerThreshold: (modeId, index, threshold) => set(s => ({
        modes: s.modes.map(m => {
          if (m.id !== modeId) return m;
          const acc = m.accel ?? { enabled: false, triggers: [] };
          const prev = index > 0 ? acc.triggers[index - 1]?.threshold : undefined;
          const allowedAfterPrev = prev == null ? [...ALLOWED_THRESHOLDS] : ALLOWED_THRESHOLDS.filter(v => v > prev);
          if (allowedAfterPrev.length === 0) return m; // nothing valid, keep as is
          const candidate = Number(threshold);
          const arr = allowedAfterPrev as ReadonlyArray<number>;
          const chosen = arr.includes(candidate) ? candidate : arr[0];

          // set current and then cascade forward to keep strictly increasing using allowed list
          const next = acc.triggers.map((curr, i) => (i === index ? { ...curr, threshold: chosen } : { ...curr }));
          for (let j = index + 1; j < next.length; j++) {
            const prevT = next[j - 1].threshold;
            const nextAllowed = ALLOWED_THRESHOLDS.find(v => v > prevT);
            if (nextAllowed == null) {
              next.splice(j); // trim trailing invalid triggers
              break;
            }
            if (next[j].threshold <= prevT) next[j].threshold = nextAllowed;
          }
           return { ...m, accel: { ...acc, triggers: next } };
        })
      })),
      setAccelTriggerWaveform: (modeId, index, waveformId) => set(s => ({
        modes: s.modes.map(m => {
          if (m.id !== modeId) return m;
          const acc = m.accel ?? { enabled: false, triggers: [] };
          const next = acc.triggers.map((t, i) => (i === index ? { ...t, waveformId } : t));
          return { ...m, accel: { ...acc, triggers: next } };
        })
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
        modes: s.modes.map(m => {
          const cleared = m.waveformId === id ? { ...m, waveformId: undefined } : m;
          const acc = cleared.accel;
          if (!acc) return cleared;
          const nextTriggers = acc.triggers.map(t => (t.waveformId === id ? { ...t, waveformId: undefined } : t));
          return { ...cleared, accel: { ...acc, triggers: nextTriggers } };
        }),
      })),

      // ModeSet library
      newModeSetDraft: () => set(() => ({
        modes: [createMode()],
        fingerOwner: createEmptyOwner(),
      })),
      saveCurrentModeSet: (name: string) => {
        const s = get();
        const modesSnap: ModeSnapshot[] = s.modes.map(m => ({
          enabled: m.enabled,
          color: m.color,
          waveformId: m.waveformId,
          accel: m.accel ? { enabled: m.accel.enabled, triggers: m.accel.triggers.map(t => ({ ...t })) } : { enabled: false, triggers: [] },
        }));
        const indexById = new Map<string, number>();
        s.modes.forEach((m, i) => indexById.set(m.id, i));
        const fingerOwnerIndex = Object.fromEntries(
          ALL_FINGERS.map(f => [f, s.fingerOwner[f] ? indexById.get(s.fingerOwner[f] as string) ?? null : null])
        ) as Record<Finger, number | null>;
        const id = nanoid(6);
        const doc: ModeSet = { id, name: name.trim() || `Set ${s.modeSets.length + 1}`, modes: modesSnap, fingerOwnerIndex };
        set(st => ({ modeSets: [...st.modeSets, doc] }));
        return id;
      },
      updateModeSet: (id: string, name?: string) => set(s => {
        const idx = s.modeSets.findIndex(x => x.id === id);
        if (idx === -1) return { modeSets: s.modeSets };
        const modesSnap: ModeSnapshot[] = s.modes.map(m => ({
          enabled: m.enabled,
          color: m.color,
          waveformId: m.waveformId,
          accel: m.accel ? { enabled: m.accel.enabled, triggers: m.accel.triggers.map(t => ({ ...t })) } : { enabled: false, triggers: [] },
        }));
        const indexById = new Map<string, number>();
        s.modes.forEach((m, i) => indexById.set(m.id, i));
        const fingerOwnerIndex = Object.fromEntries(
          ALL_FINGERS.map(f => [f, s.fingerOwner[f] ? indexById.get(s.fingerOwner[f] as string) ?? null : null])
        ) as Record<Finger, number | null>;
        const existing = s.modeSets[idx];
        const updated: ModeSet = {
          ...existing,
          name: name != null ? name : existing.name,
          modes: modesSnap,
          fingerOwnerIndex,
        };
        const next = [...s.modeSets];
        next[idx] = updated;
        return { modeSets: next };
      }),
      loadModeSet: (id: string) => set(s => {
        const doc = s.modeSets.find(x => x.id === id);
        if (!doc) return { modes: s.modes, fingerOwner: s.fingerOwner };
        const newModes = doc.modes.map(ms => createMode({
          enabled: ms.enabled,
          color: ms.color,
          waveformId: ms.waveformId,
          accel: ms.accel ? { enabled: ms.accel.enabled, triggers: ms.accel.triggers.map(t => ({ ...t })) } : { enabled: false, triggers: [] },
        }));
        const newFingerOwner: Record<Finger, string | null> = createEmptyOwner();
        for (const f of ALL_FINGERS) {
          const idx = doc.fingerOwnerIndex[f];
          newFingerOwner[f] = idx == null ? null : newModes[idx]?.id ?? null;
        }
        return { modes: newModes, fingerOwner: newFingerOwner };
      }),
      renameModeSet: (id: string, name: string) => set(s => ({
        modeSets: s.modeSets.map(d => (d.id === id ? { ...d, name } : d)),
      })),
      removeModeSet: (id: string) => set(s => ({
        modeSets: s.modeSets.filter(d => d.id !== id),
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
          enabled: m.enabled,
          color: m.color,
          waveformId: m.waveformId,
          waveform: m.waveformId ? s.waveforms.find(w => w.id === m.waveformId) ?? null : null,
          fingers: ALL_FINGERS.filter(f => s.fingerOwner[f] === m.id),
          accel: m.accel ? {
            enabled: m.accel.enabled,
            triggers: m.accel.triggers.map(t => ({
              threshold: t.threshold,
              waveformId: t.waveformId,
              waveform: t.waveformId ? s.waveforms.find(w => w.id === t.waveformId) ?? null : null,
            })),
          } : { enabled: false, triggers: [] },
        }));
        console.log('SEND', payload);
      },
    }),
    { name: 'bulbchips-store' }
  )
);
