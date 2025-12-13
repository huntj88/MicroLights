import { create } from 'zustand';
import { persist } from 'zustand/middleware';

export type ThemePreference = 'light' | 'dark' | 'system';
export type ThemeValue = 'light' | 'dark';

export interface ThemeStore {
  preference: ThemePreference;
  setPreference: (preference: ThemePreference) => void;
}

export const useThemeStore = create<ThemeStore>()(
  persist(
    set => ({
      preference: 'system',
      setPreference: preference => set({ preference }),
    }),
    {
      name: 'flow-art-forge-theme',
      version: 1,
      partialize: state => ({ preference: state.preference }),
    },
  ),
);

export const resolveThemeValue = (
  preference: ThemePreference,
  systemTheme: ThemeValue,
): ThemeValue => (preference === 'system' ? systemTheme : preference);
