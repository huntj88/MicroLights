import { create } from 'zustand';
import { persist } from 'zustand/middleware';

import { type Mode } from '../models/mode';

export interface ModeStoreState {
  modes: Mode[];
  saveMode: (mode: Mode) => void;
  deleteMode: (name: string) => void;
  getMode: (name: string) => Mode | undefined;
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
    }),
    {
      name: 'mode-storage',
    },
  ),
);
