import { create } from 'zustand';
import { persist } from 'zustand/middleware';

import { type Mode, type ModePattern } from '../models/mode';

export interface ModeStoreState {
  modes: Mode[];
  saveMode: (mode: Mode) => void;
  deleteMode: (name: string) => void;
  getMode: (name: string) => Mode | undefined;
  updatePatternInModes: (pattern: ModePattern) => void;
}

// Deep clone helper to ensure we don't store references to mutable objects
const cloneMode = (mode: Mode): Mode => {
  return JSON.parse(JSON.stringify(mode)) as Mode;
};

export const useModeStore = create<ModeStoreState>()(
  persist(
    (set, get) => ({
      modes: [],
      saveMode: mode =>
        set(state => {
          const existingIndex = state.modes.findIndex(entry => entry.name === mode.name);
          const nextMode = cloneMode(mode);

          if (existingIndex === -1) {
            return {
              modes: [...state.modes, nextMode],
            };
          }

          const nextModes = [...state.modes];
          nextModes.splice(existingIndex, 1, nextMode);
          return { modes: nextModes };
        }),
      deleteMode: name =>
        set(state => ({
          modes: state.modes.filter(p => p.name !== name),
        })),
      getMode: name => get().modes.find(p => p.name === name),
      updatePatternInModes: pattern =>
        set(state => {
          const nextModes = state.modes.map(mode => {
            const nextMode = cloneMode(mode);

            // Helper to update a component if it matches the pattern
            const updateComponent = (component: { pattern: ModePattern } | undefined) => {
              if (component?.pattern.name === pattern.name) {
                component.pattern = pattern;
              }
            };

            updateComponent(nextMode.front);
            updateComponent(nextMode.case);

            if (nextMode.accel?.triggers) {
              for (const trigger of nextMode.accel.triggers) {
                updateComponent(trigger.front);
                updateComponent(trigger.case);
              }
            }

            return nextMode;
          });

          return { modes: nextModes };
        }),
    }),
    {
      name: 'mode-storage',
    },
  ),
);
