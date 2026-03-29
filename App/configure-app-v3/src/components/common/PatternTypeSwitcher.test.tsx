import { describe, expect, it } from 'vitest';

import { renderWithProviders, screen } from '@/test-utils/render-with-providers';

import { PatternTypeSwitcher } from './PatternTypeSwitcher';

describe('PatternTypeSwitcher', () => {
  it('renders both RGB and Bulb links', () => {
    renderWithProviders(<PatternTypeSwitcher />);

    expect(screen.getByText('nav.rgbPattern')).toBeInTheDocument();
    expect(screen.getByText('nav.bulbPattern')).toBeInTheDocument();
  });

  it('has a nav element with the patterns aria-label', () => {
    renderWithProviders(<PatternTypeSwitcher />);

    expect(screen.getByRole('navigation', { name: 'nav.patterns' })).toBeInTheDocument();
  });

  it('links to the correct routes', () => {
    renderWithProviders(<PatternTypeSwitcher />);

    const rgbLink = screen.getByText('nav.rgbPattern').closest('a');
    const bulbLink = screen.getByText('nav.bulbPattern').closest('a');

    expect(rgbLink).toHaveAttribute('href', '/patterns/rgb');
    expect(bulbLink).toHaveAttribute('href', '/patterns/bulb');
  });

  it('highlights the RGB tab when on the RGB pattern route', () => {
    renderWithProviders(<PatternTypeSwitcher />, {
      routerEntries: ['/patterns/rgb'],
    });

    const rgbLink = screen.getByText('nav.rgbPattern').closest('a');
    const bulbLink = screen.getByText('nav.bulbPattern').closest('a');

    expect(rgbLink?.className).toContain('bg-[rgb(var(--accent)');
    expect(bulbLink?.className).not.toContain('bg-[rgb(var(--accent)');
  });

  it('highlights the Bulb tab when on the Bulb pattern route', () => {
    renderWithProviders(<PatternTypeSwitcher />, {
      routerEntries: ['/patterns/bulb'],
    });

    const rgbLink = screen.getByText('nav.rgbPattern').closest('a');
    const bulbLink = screen.getByText('nav.bulbPattern').closest('a');

    expect(bulbLink?.className).toContain('bg-[rgb(var(--accent)');
    expect(rgbLink?.className).not.toContain('bg-[rgb(var(--accent)');
  });

  it('defaults to Bulb highlight on a non-pattern route', () => {
    renderWithProviders(<PatternTypeSwitcher />, {
      routerEntries: ['/modes'],
    });

    const rgbLink = screen.getByText('nav.rgbPattern').closest('a');
    const bulbLink = screen.getByText('nav.bulbPattern').closest('a');

    // When not on RGB route, isRgb is false so Bulb side is highlighted
    expect(bulbLink?.className).toContain('bg-[rgb(var(--accent)');
    expect(rgbLink?.className).not.toContain('bg-[rgb(var(--accent)');
  });
});
