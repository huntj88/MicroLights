import { nanoid } from 'nanoid/non-secure';
import { create } from 'zustand';
import { persist } from 'zustand/middleware';

import { DISABLED_COLOR } from './constants';
import { DEFAULT_WAVEFORMS } from './defaultWaveforms';
import { ALL_FINGERS, type Finger } from './fingers';
import type { Waveform } from './waveform';

// Allowed accelerometer threshold values
export const ALLOWED_THRESHOLDS = [2, 4, 8, 12, 16] as const;

// Shared type for accelerometer triggers
export type Trigger = {
  threshold: number;
  color?: string; // hex; defaults to mode color if missing
  waveformId?: string;
};

export type Mode = {
  id: string;
  modeSetId?: string | null;
  color: string; // hex
  waveformId?: string;
  accel?: {
    triggers: Array<Trigger>;
  };
};

export type WaveformDoc = { id: string; readonly?: boolean } & Waveform;

// JSON export shape without UI state or ids
export type ExportedMode = {
  name: string;
  color: string;
  waveform?: Waveform;
  accel: {
    triggers: Array<{
      threshold: Trigger['threshold'];
      color: string;
      waveform?: Waveform;
    }>;
  };
};

// A saved ModeSet snapshot of modes and finger ownership
export type ModeSnapshot = Pick<Mode, 'color' | 'waveformId' | 'accel'>;
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

  waveforms: WaveformDoc[];
  modeSets: ModeSet[];

  // tracks the last applied defaults signature to know when to resync
  defaultWaveformsSignature: string;

  lastSelectedModeSetId: string | null;

  // UI settings
  theme: 'system' | 'light' | 'dark';

  // actions
  addMode: (partial?: Partial<Mode>) => string;
  removeMode: (modeId: string) => void;

  // theme actions
  setThemePreference: (pref: 'system' | 'light' | 'dark') => void;

  assignFinger: (modeId: string, finger: Finger) => void;
  unassignFinger: (modeId: string, finger: Finger) => void;
  selectAll: (modeId: string) => void;

  setWaveform: (modeId: string, waveformId?: string) => void;
  setColor: (modeId: string, hex: string) => void;

  // accelerometer actions
  addAccelTrigger: (modeId: string) => void; // max 2
  removeAccelTrigger: (modeId: string, index: number) => void;
  setAccelTriggerThreshold: (modeId: string, index: number, threshold: number) => void;
  setAccelTriggerWaveform: (modeId: string, index: number, waveformId?: string) => void;
  setAccelTriggerColor: (modeId: string, index: number, color: string) => void;

  addWaveform: (wf: Waveform) => string;
  updateWaveform: (id: string, wf: Partial<Waveform>) => void;
  removeWaveform: (id: string) => void;

  // mode set library actions
  newModeSetDraft: () => void; // reset current working set
  saveCurrentModeSet: (name: string) => string; // returns set id
  updateModeSet: (id: string, name?: string) => void; // overwrite existing set with current state
  loadModeSet: (id: string) => void;
  removeModeSet: (id: string) => void;

  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  send: () => Promise<void>;
  exportModeAsJSON: (modeId: string) => string;
};

const createEmptyOwner = (): Record<Finger, string | null> =>
  Object.fromEntries(ALL_FINGERS.map(f => [f, null])) as Record<Finger, string | null>;

const createMode = (partial?: Partial<Mode>): Mode => ({
  id: nanoid(6),
  modeSetId: null,
  color: DISABLED_COLOR,
  accel: { triggers: [] },
  ...partial,
});

export const useAppStore = create<AppState>()(
  persist(
    (set, get) => ({
      modes: [createMode()],
      fingerOwner: createEmptyOwner(),
      connected: false,

      // default to system theme; AppShell computes actual dark/light
      theme: 'system',

      waveforms: DEFAULT_WAVEFORMS.map(wf => ({ id: nanoid(6), readonly: true, ...wf })),
      defaultWaveformsSignature: JSON.stringify(DEFAULT_WAVEFORMS),

      modeSets: [],
      lastSelectedModeSetId: null,

      addMode: partial => {
        const mode = createMode({ ...partial });
        set(s => ({ modes: [...s.modes, mode] }));
        return mode.id;
      },
      removeMode: modeId => {
        set(s => ({
          modes: s.modes.filter(m => m.id !== modeId),
          fingerOwner: Object.fromEntries(
            Object.entries(s.fingerOwner).map(([f, owner]) => [f, owner === modeId ? null : owner]),
          ) as Record<Finger, string | null>,
        }));
      },

      // theme
      setThemePreference: pref => set(() => ({ theme: pref })),

      assignFinger: (modeId, finger) =>
        set(s => {
          const nextOwner = { ...s.fingerOwner };
          // Unassign from previous owner if any
          if (nextOwner[finger] && nextOwner[finger] !== modeId) {
            nextOwner[finger] = null;
          }
          nextOwner[finger] = modeId;
          return { fingerOwner: nextOwner };
        }),
      unassignFinger: (modeId, finger) =>
        set(s => {
          if (s.fingerOwner[finger] !== modeId) return { fingerOwner: s.fingerOwner };
          return { fingerOwner: { ...s.fingerOwner, [finger]: null } };
        }),
      selectAll: modeId =>
        set(s => {
          const next = { ...s.fingerOwner };
          for (const f of ALL_FINGERS) next[f] = modeId;
          return { fingerOwner: next };
        }),

      setWaveform: (modeId, waveformId) =>
        set(s => ({
          modes: s.modes.map(m => (m.id === modeId ? { ...m, waveformId } : m)),
        })),
      setColor: (modeId, hex) =>
        set(s => ({
          modes: s.modes.map(m => (m.id === modeId ? { ...m, color: hex } : m)),
        })),
      // case light enablement is inferred: color === DISABLED_COLOR means disabled

      // accelerometer
      addAccelTrigger: modeId =>
        set(s => ({
          modes: s.modes.map(m => {
            if (m.id !== modeId) return m;
            const acc = m.accel ?? { triggers: [] };
            if (acc.triggers.length >= 2) return m;
            const prev = acc.triggers[acc.triggers.length - 1];
            const nextAllowed = prev
              ? ALLOWED_THRESHOLDS.find(v => v > prev.threshold)
              : ALLOWED_THRESHOLDS[0];
            if (nextAllowed == null) return m; // no valid higher threshold available
            return {
              ...m,
              accel: {
                ...acc,
                triggers: [
                  ...acc.triggers,
                  { threshold: nextAllowed, color: m.color, waveformId: undefined },
                ],
              },
            };
          }),
        })),
      removeAccelTrigger: (modeId, index) =>
        set(s => ({
          modes: s.modes.map(m => {
            if (m.id !== modeId) return m;
            const acc = m.accel ?? { triggers: [] };
            const next = acc.triggers.filter((_, i) => i !== index);
            return { ...m, accel: { ...acc, triggers: next } };
          }),
        })),
      setAccelTriggerThreshold: (modeId, index, threshold) =>
        set(s => ({
          modes: s.modes.map(m => {
            if (m.id !== modeId) return m;
            const acc = m.accel ?? { triggers: [] };
            const prev = index > 0 ? acc.triggers[index - 1]?.threshold : undefined;
            const allowedAfterPrev =
              prev == null ? [...ALLOWED_THRESHOLDS] : ALLOWED_THRESHOLDS.filter(v => v > prev);
            if (allowedAfterPrev.length === 0) return m; // nothing valid, keep as is
            const candidate = Number(threshold);
            const arr = allowedAfterPrev as ReadonlyArray<number>;
            const chosen = arr.includes(candidate) ? candidate : arr[0];

            // set current and then cascade forward to keep strictly increasing using allowed list
            const next = acc.triggers.map((curr, i) =>
              i === index ? { ...curr, threshold: chosen } : { ...curr },
            );
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
          }),
        })),
      setAccelTriggerWaveform: (modeId, index, waveformId) =>
        set(s => ({
          modes: s.modes.map(m => {
            if (m.id !== modeId) return m;
            const acc = m.accel ?? { triggers: [] };
            const next = acc.triggers.map((t, i) => (i === index ? { ...t, waveformId } : t));
            return { ...m, accel: { ...acc, triggers: next } };
          }),
        })),
      setAccelTriggerColor: (modeId, index, color) =>
        set(s => ({
          modes: s.modes.map(m => {
            if (m.id !== modeId) return m;
            const acc = m.accel ?? { triggers: [] };
            const next = acc.triggers.map((t, i) => (i === index ? { ...t, color } : t));
            return { ...m, accel: { ...acc, triggers: next } };
          }),
        })),

      addWaveform: wf => {
        const id = nanoid(6);
        set(s => ({ waveforms: [...s.waveforms, { id, ...wf }] }));
        return id;
      },
      updateWaveform: (id, wf) =>
        set(s => {
          const target = s.waveforms.find(x => x.id === id);
          if (!target || target.readonly) return { waveforms: s.waveforms };
          return { waveforms: s.waveforms.map(x => (x.id === id ? { ...x, ...wf } : x)) };
        }),
      removeWaveform: id =>
        set(s => {
          const target = s.waveforms.find(x => x.id === id);
          if (!target || target.readonly) return { waveforms: s.waveforms };
          return {
            waveforms: s.waveforms.filter(x => x.id !== id),
            modes: s.modes.map(m => {
              const cleared = m.waveformId === id ? { ...m, waveformId: undefined } : m;
              const acc = cleared.accel;
              if (!acc) return cleared;
              const nextTriggers = acc.triggers.map(t =>
                t.waveformId === id ? { ...t, waveformId: undefined } : t,
              );
              return { ...cleared, accel: { ...acc, triggers: nextTriggers } };
            }),
          };
        }),

      // ModeSet library
      newModeSetDraft: () =>
        set(() => ({
          modes: [createMode()],
          fingerOwner: createEmptyOwner(),
          lastSelectedModeSetId: null,
        })),
      saveCurrentModeSet: (name: string) => {
        const s = get();
        const modesSnap: ModeSnapshot[] = s.modes.map(m => ({
          color: m.color,
          waveformId: m.waveformId,
          accel: m.accel ? { triggers: m.accel.triggers.map(t => ({ ...t })) } : { triggers: [] },
        }));
        const indexById = new Map<string, number>();
        s.modes.forEach((m, i) => indexById.set(m.id, i));
        const fingerOwnerIndex = Object.fromEntries(
          ALL_FINGERS.map(f => [
            f,
            s.fingerOwner[f] ? (indexById.get(s.fingerOwner[f] as string) ?? null) : null,
          ]),
        ) as Record<Finger, number | null>;
        const id = nanoid(6);
        const doc: ModeSet = {
          id,
          name: name.trim() || `Set ${s.modeSets.length + 1}`,
          modes: modesSnap,
          fingerOwnerIndex,
        };
        set(st => ({
          modeSets: [...st.modeSets, doc],
          modes: st.modes.map(m => ({ ...m, modeSetId: id })),
          lastSelectedModeSetId: id,
        }));
        return id;
      },
      updateModeSet: (id: string, name?: string) =>
        set(s => {
          const idx = s.modeSets.findIndex(x => x.id === id);
          if (idx === -1) return { modeSets: s.modeSets };
          const modesSnap: ModeSnapshot[] = s.modes.map(m => ({
            color: m.color,
            waveformId: m.waveformId,
            accel: m.accel ? { triggers: m.accel.triggers.map(t => ({ ...t })) } : { triggers: [] },
          }));
          const indexById = new Map<string, number>();
          s.modes.forEach((m, i) => indexById.set(m.id, i));
          const fingerOwnerIndex = Object.fromEntries(
            ALL_FINGERS.map(f => [
              f,
              s.fingerOwner[f] ? (indexById.get(s.fingerOwner[f] as string) ?? null) : null,
            ]),
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
          return { modeSets: next, lastSelectedModeSetId: id };
        }),
      loadModeSet: (id: string) =>
        set(s => {
          const doc = s.modeSets.find(x => x.id === id);
          if (!doc) return { modes: s.modes, fingerOwner: s.fingerOwner };
          const newModes = doc.modes.map(ms =>
            createMode({
              modeSetId: id,
              color: ms.color,
              waveformId: ms.waveformId,
              accel: ms.accel
                ? { triggers: ms.accel.triggers.map(t => ({ ...t })) }
                : { triggers: [] },
            }),
          );
          const newFingerOwner: Record<Finger, string | null> = createEmptyOwner();
          for (const f of ALL_FINGERS) {
            const idx = doc.fingerOwnerIndex[f];
            newFingerOwner[f] = idx == null ? null : (newModes[idx]?.id ?? null);
          }
          return { modes: newModes, fingerOwner: newFingerOwner, lastSelectedModeSetId: id };
        }),
      removeModeSet: (id: string) =>
        set(s => {
          const nextSets = s.modeSets.filter(d => d.id !== id);
          const nextSelected =
            s.lastSelectedModeSetId === id
              ? nextSets.length > 0
                ? nextSets[nextSets.length - 1]!.id
                : null
              : s.lastSelectedModeSetId;
          return {
            modeSets: nextSets,
            modes: s.modes.map(m => (m.modeSetId === id ? { ...m, modeSetId: null } : m)),
            lastSelectedModeSetId: nextSelected,
          };
        }),

      connect: async () => {
        set({ connected: true });
      },
      disconnect: async () => {
        set({ connected: false });
      },

      exportModeAsJSON: modeId => {
        const s = get();
        const m = s.modes.find(x => x.id === modeId);
        if (!m) return JSON.stringify(null);

        const inline = (id?: string) => {
          if (!id) return undefined;
          const doc = s.waveforms.find(w => w.id === id);
          if (!doc) return undefined;
          const { id: _omit, readonly: _omitRO, ...wf } = doc; // strip id, readonly
          return wf;
        };

        const modeSetName =
          (m.modeSetId && s.modeSets.find(d => d.id === m.modeSetId)?.name) ?? 'Draft';

        const triggers = m.accel?.triggers ?? [];
        const payload: ExportedMode = {
          name: modeSetName,
          color: m.color,
          waveform: inline(m.waveformId),
          accel: {
            triggers: triggers.map(t => ({
              threshold: t.threshold,
              color: t.color ?? m.color,
              waveform: inline(t.waveformId),
            })),
          },
        };

        return JSON.stringify(payload, null, 2);
      },

      send: async () => {
        console.log('SEND');
      },
    }),
    {
      name: 'bulbchips-store',
      version: 1,
      onRehydrateStorage: () => (state, _error) => {
        // After rehydrate, sync readonly defaults with the current DEFAULT_WAVEFORMS
        try {
          const currentSig = JSON.stringify(DEFAULT_WAVEFORMS);
          const s = (state as unknown as AppState) ?? undefined;
          if (!s) return;
          const needsSync = s.defaultWaveformsSignature !== currentSig;
          if (!needsSync) return;

          const slug = (name: string) =>
            name
              .toLowerCase()
              .replace(/[^a-z0-9]+/g, '-')
              .replace(/(^-|-$)/g, '');

          const desired = new Map(DEFAULT_WAVEFORMS.map(wf => [slug(wf.name), wf as Waveform]));
          const existingReadonly = new Map(
            s.waveforms.filter(w => w.readonly).map(w => [slug(w.name), w as WaveformDoc]),
          );

          const keepOrUpdated: WaveformDoc[] = [];
          // Update existing presets to match defaults
          for (const [key, wf] of desired) {
            const doc = existingReadonly.get(key);
            if (doc) {
              keepOrUpdated.push({
                id: doc.id,
                readonly: true,
                name: wf.name,
                totalTicks: wf.totalTicks,
                changeAt: wf.changeAt,
              });
            }
          }
          // Add new presets
          for (const [key, wf] of desired) {
            if (!existingReadonly.has(key)) {
              keepOrUpdated.push({
                id: nanoid(6),
                readonly: true,
                name: wf.name,
                totalTicks: wf.totalTicks,
                changeAt: wf.changeAt,
              });
            }
          }
          // Determine removed preset ids
          const desiredKeys = new Set(desired.keys());
          const removedIds = s.waveforms
            .filter(w => w.readonly && !desiredKeys.has(slug(w.name)))
            .map(w => w.id);

          // Build next waveforms: non-readonly unchanged + synced readonly
          const nonReadonly = s.waveforms.filter(w => !w.readonly);
          const nextWaveforms = [...nonReadonly, ...keepOrUpdated];

          // Clear references to removed ids in modes and triggers
          const nextModes = s.modes.map(m => {
            const cleared = removedIds.includes(m.waveformId ?? '')
              ? { ...m, waveformId: undefined }
              : m;
            const acc = cleared.accel;
            if (!acc) return cleared;
            const nextTriggers = acc.triggers.map(t =>
              removedIds.includes(t.waveformId ?? '') ? { ...t, waveformId: undefined } : t,
            );
            return { ...cleared, accel: { ...acc, triggers: nextTriggers } };
          });

          // Apply
          const target = state as unknown as AppState;
          target.waveforms = nextWaveforms;
          target.modes = nextModes;
          target.defaultWaveformsSignature = currentSig;
        } catch (e) {
          console.error('Failed to sync default waveforms:', e);
        }
      },
    },
  ),
);
