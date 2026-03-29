import { describe, expect, it } from 'vitest';

import { renderWithProviders, screen } from '@/test-utils/render-with-providers';

import { MobileBottomNav } from './MobileBottomNav';

describe('MobileBottomNav', () => {
  it('renders three navigation tabs', () => {
    renderWithProviders(
      <MobileBottomNav />,
      { routerEntries: ['/'] },
    );

    expect(screen.getByText('nav.patterns')).toBeInTheDocument();
    expect(screen.getByText('nav.mode')).toBeInTheDocument();
    expect(screen.getByText('nav.serialLog')).toBeInTheDocument();
  });

  it('marks the active tab with aria-current', () => {
    renderWithProviders(
      <MobileBottomNav />,
      { routerEntries: ['/patterns/rgb'] },
    );

    const patternsLink = screen.getByText('nav.patterns').closest('a');
    expect(patternsLink).toHaveAttribute('aria-current', 'page');
  });

  it('does not include overview or settings', () => {
    renderWithProviders(
      <MobileBottomNav />,
      { routerEntries: ['/'] },
    );

    expect(screen.queryByText('nav.home')).not.toBeInTheDocument();
    expect(screen.queryByText('nav.settings')).not.toBeInTheDocument();
  });
});
