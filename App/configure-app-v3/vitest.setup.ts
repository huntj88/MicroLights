import '@testing-library/jest-dom/vitest';
import 'vitest-canvas-mock';
import { vi } from 'vitest';

// Mock ResizeObserver for jsdom (not available natively)
(globalThis as unknown as Record<string, unknown>).ResizeObserver = class ResizeObserver {
  private cb: ResizeObserverCallback;
  constructor(cb: ResizeObserverCallback) {
    this.cb = cb;
  }
  observe() {
    // Fire callback once with a stub entry so components can initialize
    this.cb([{ contentRect: { width: 300, height: 100 } } as ResizeObserverEntry], this);
  }
  unobserve() {
    /* noop */
  }
  disconnect() {
    /* noop */
  }
};

vi.mock('react-i18next', () => ({
  useTranslation: () => ({
    t: (key: string) => key,
    i18n: {
      changeLanguage: () =>
        new Promise(() => {
          /* ignore */
        }),
    },
  }),
  initReactI18next: {
    type: '3rdParty',
    init: () => {
      /* ignore */
    },
  },
  I18nextProvider: ({ children }: { children: React.ReactNode }) => children,
  Trans: ({ children }: { children: React.ReactNode }) => children,
}));
