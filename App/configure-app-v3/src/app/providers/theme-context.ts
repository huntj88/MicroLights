import { createContext, useContext } from 'react';

import type { ThemePreference, ThemeValue } from './theme-store';

export interface ThemeContextValue {
  preference: ThemePreference;
  resolved: ThemeValue;
  setPreference: (preference: ThemePreference) => void;
}

export const ThemeContext = createContext<ThemeContextValue | undefined>(undefined);

export const useTheme = () => {
  const context = useContext(ThemeContext);

  if (!context) {
    throw new Error('useTheme must be used within ThemeProvider');
  }

  return context;
};
