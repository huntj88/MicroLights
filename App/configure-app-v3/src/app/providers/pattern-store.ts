import { z } from 'zod';
import { create } from 'zustand';
import { persist } from 'zustand/middleware';

import { useModeStore } from './mode-store';
import { modePatternSchema, type ModePattern } from '../models/mode';

export interface PatternStoreState {
  patterns: ModePattern[];
  savePattern: (pattern: ModePattern) => void;
  deletePattern: (name: string) => void;
  getPattern: (name: string) => ModePattern | undefined;
}

const clonePattern = (pattern: ModePattern): ModePattern => {
  if (pattern.type === 'equation') {
    return {
      ...pattern,
      red: { ...pattern.red, sections: [...pattern.red.sections.map(s => ({ ...s }))] },
      green: { ...pattern.green, sections: [...pattern.green.sections.map(s => ({ ...s }))] },
      blue: { ...pattern.blue, sections: [...pattern.blue.sections.map(s => ({ ...s }))] },
    };
  }
  return {
    ...pattern,
    changeAt: pattern.changeAt.map(change => ({
      ...change,
    })),
  };
};

export const usePatternStore = create<PatternStoreState>()(
  persist(
    (set, get) => ({
      patterns: [],
      savePattern: pattern => {
        set(state => {
          const existingIndex = state.patterns.findIndex(entry => entry.name === pattern.name);
          const nextPattern = clonePattern(pattern);

          if (existingIndex === -1) {
            return {
              patterns: [...state.patterns, nextPattern],
            };
          }

          const nextPatterns = [...state.patterns];
          nextPatterns.splice(existingIndex, 1, nextPattern);
          return { patterns: nextPatterns };
        });

        // Propagate changes to modes
        useModeStore.getState().updatePatternInModes(pattern);
      },
      deletePattern: name =>
        set(state => ({
          patterns: state.patterns.filter(pattern => pattern.name !== name),
        })),
      getPattern: name => {
        const pattern = get().patterns.find(entry => entry.name === name);
        return pattern ? clonePattern(pattern) : undefined;
      },
    }),
    {
      name: 'flow-art-forge-patterns',
      version: 1,
      partialize: state => ({ patterns: state.patterns }),
      merge: (persistedState, currentState) => {
        const schema = z.object({
          patterns: z.array(modePatternSchema),
        });
        const result = schema.safeParse(persistedState);
        if (result.success) {
          return { ...currentState, patterns: result.data.patterns };
        }
        return currentState;
      },
    },
  ),
);
