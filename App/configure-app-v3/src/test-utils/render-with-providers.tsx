/* eslint-disable react-refresh/only-export-components */
import { render, type RenderOptions } from '@testing-library/react';
import type { PropsWithChildren, ReactElement } from 'react';
import { I18nextProvider } from 'react-i18next';
import { MemoryRouter } from 'react-router-dom';

import { i18n } from '@/app/providers/i18n';
import { ThemeProvider } from '@/app/providers/ThemeProvider';

interface ExtendedRenderOptions extends RenderOptions {
  /** Initial route entries for MemoryRouter (default: ['/']) */
  routerEntries?: string[];
}

const createProviders = (routerEntries: string[]) => {
  const Providers = ({ children }: PropsWithChildren): ReactElement => (
    <MemoryRouter initialEntries={routerEntries}>
      <I18nextProvider i18n={i18n}>
        <ThemeProvider>{children}</ThemeProvider>
      </I18nextProvider>
    </MemoryRouter>
  );
  return Providers;
};

export const renderWithProviders = (ui: ReactElement, options?: ExtendedRenderOptions) => {
  const { routerEntries = ['/'], ...renderOptions } = options ?? {};
  return render(ui, { wrapper: createProviders(routerEntries), ...renderOptions });
};

/**
 * Render with viewport set to a mobile width (375px).
 * Useful for testing responsive behavior in jsdom.
 * Restores the previous innerWidth via the returned `cleanup` helper.
 */
export const renderMobile = (ui: ReactElement, options?: ExtendedRenderOptions) => {
  const prev = window.innerWidth;
  Object.defineProperty(window, 'innerWidth', { value: 375, writable: true, configurable: true });
  window.dispatchEvent(new Event('resize'));
  const result = renderWithProviders(ui, options);
  return {
    ...result,
    cleanup: () => {
      Object.defineProperty(window, 'innerWidth', {
        value: prev,
        writable: true,
        configurable: true,
      });
      window.dispatchEvent(new Event('resize'));
    },
  };
};

/**
 * Render with viewport set to a desktop width (1280px).
 * Useful for testing responsive behavior in jsdom.
 * Restores the previous innerWidth via the returned `cleanup` helper.
 */
export const renderDesktop = (ui: ReactElement, options?: ExtendedRenderOptions) => {
  const prev = window.innerWidth;
  Object.defineProperty(window, 'innerWidth', { value: 1280, writable: true, configurable: true });
  window.dispatchEvent(new Event('resize'));
  const result = renderWithProviders(ui, options);
  return {
    ...result,
    cleanup: () => {
      Object.defineProperty(window, 'innerWidth', {
        value: prev,
        writable: true,
        configurable: true,
      });
      window.dispatchEvent(new Event('resize'));
    },
  };
};

/**
 * Touch-target audit: asserts that all interactive elements in the container
 * have min-height and min-width CSS classes of at least 44px.
 *
 * This checks for Tailwind min-h-[44px] / min-w-[44px] / min-h-[48px] etc. class presence,
 * since jsdom doesn't compute real layout sizes.
 *
 * Returns an array of elements that fail the check (empty = all pass).
 */
export const auditTouchTargets = (container: HTMLElement): HTMLElement[] => {
  const interactiveSelectors = 'button, a[href], input, select, textarea, [role="button"]';
  const elements = Array.from(container.querySelectorAll<HTMLElement>(interactiveSelectors));

  const MIN_TOUCH_PX = 44;

  /** Check whether the class list contains a min-h/min-w or h-/w- class >= minPx */
  const meetsSize = (classes: string, axis: 'h' | 'w'): boolean => {
    // Match min-h-[Npx] / min-w-[Npx]
    const arbRegex = axis === 'h' ? /\bmin-h-\[(\d+)px\]/g : /\bmin-w-\[(\d+)px\]/g;
    for (const m of classes.matchAll(arbRegex)) {
      if (parseInt(m[1], 10) >= MIN_TOUCH_PX) return true;
    }
    // Match Tailwind scale utilities h-11 (44px), h-12 (48px), etc.
    const scaleRegex = axis === 'h' ? /\bh-(1[1-9]|[2-9]\d)/ : /\bw-(1[1-9]|[2-9]\d)/;
    return scaleRegex.test(classes);
  };

  return elements.filter(el => {
    const classes = el.className;
    const hasMinHeight = meetsSize(classes, 'h');
    const hasMinWidth = meetsSize(classes, 'w');
    // Inline inputs only need sufficient height
    const isInlineInput =
      el.tagName === 'INPUT' || el.tagName === 'SELECT' || el.tagName === 'TEXTAREA';
    return isInlineInput ? !hasMinHeight : !(hasMinHeight && hasMinWidth);
  });
};

export * from '@testing-library/react';
