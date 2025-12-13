import userEvent from '@testing-library/user-event';
import { beforeEach, describe, expect, it, vi } from 'vitest';

import { renderWithProviders, screen } from '@/test-utils/render-with-providers';

import { ThemePreferenceSelector } from './ThemePreferenceSelector';
import type { ThemePreferenceState } from './ThemePreferenceSelector';

describe('ThemePreferenceSelector', () => {
  const baseState: ThemePreferenceState = {
    mode: 'system',
    resolved: 'light',
  };

  beforeEach(() => {
    window.localStorage.clear();
  });

  it('renders the current selection', () => {
    renderWithProviders(<ThemePreferenceSelector onChange={vi.fn()} value={baseState} />);

    const systemRadio = screen.getByRole('radio', { name: /follow system/i });
    expect(systemRadio).toBeChecked();
  });

  it('emits new state when selecting another option', async () => {
    const user = userEvent.setup();
    const handleChange = vi.fn();

    renderWithProviders(<ThemePreferenceSelector onChange={handleChange} value={baseState} />);

    await user.click(screen.getByRole('radio', { name: /light/i }));

    expect(handleChange).toHaveBeenCalledWith(
      {
        mode: 'light',
        resolved: baseState.resolved,
      },
      { type: 'change-mode', mode: 'light' },
    );
  });
});
