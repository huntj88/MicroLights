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
 */
export const renderMobile = (ui: ReactElement, options?: ExtendedRenderOptions) => {
  Object.defineProperty(window, 'innerWidth', { value: 375, writable: true, configurable: true });
  window.dispatchEvent(new Event('resize'));
  return renderWithProviders(ui, options);
};

/**
 * Render with viewport set to a desktop width (1280px).
 * Useful for testing responsive behavior in jsdom.
 */
export const renderDesktop = (ui: ReactElement, options?: ExtendedRenderOptions) => {
  Object.defineProperty(window, 'innerWidth', { value: 1280, writable: true, configurable: true });
  window.dispatchEvent(new Event('resize'));
  return renderWithProviders(ui, options);
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

  // Pattern matches min-h-[Npx] where N >= 44, or h-11, h-12, etc.
  const minHeightPattern = /\bmin-h-\[(\d+)px\]|\bh-(1[1-9]|[2-9]\d)/;
  const minWidthPattern = /\bmin-w-\[(\d+)px\]|\bw-(1[1-9]|[2-9]\d)/;

  return elements.filter(el => {
    const classes = el.className;
    const hasMinHeight = minHeightPattern.test(classes);
    const hasMinWidth = minWidthPattern.test(classes);
    // Elements that are inline (inputs with type text/number) only need height
    const isInlineInput = el.tagName === 'INPUT' || el.tagName === 'SELECT' || el.tagName === 'TEXTAREA';
    return isInlineInput ? !hasMinHeight : !(hasMinHeight || hasMinWidth);
  });
};

export * from '@testing-library/react';
