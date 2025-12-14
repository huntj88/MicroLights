import userEvent from '@testing-library/user-event';
import { describe, expect, it } from 'vitest';

import { hexColorSchema, type SimplePattern } from '@/app/models/mode';
import { usePatternStore } from '@/app/providers/pattern-store';
import { renderWithProviders, screen, within } from '@/test-utils/render-with-providers';

import { BulbPatternPage } from './BulbPatternPage';

describe('BulbPatternPage', () => {
  const setup = () => {
    usePatternStore.setState({ patterns: [] });
    localStorage.removeItem('flow-art-forge-patterns');
    return {
      user: userEvent.setup(),
    };
  };

  it('renders the bulb pattern editor', () => {
    setup();
    renderWithProviders(<BulbPatternPage />);

    expect(screen.getByRole('heading', { level: 2, name: /bulb pattern studio/i })).toBeInTheDocument();
  });

  it('filters out color patterns from the saved patterns list', () => {
    setup();
    const binaryPattern: SimplePattern = {
      type: 'simple',
      name: 'Binary Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: 'high' }],
    };
    const colorPattern: SimplePattern = {
      type: 'simple',
      name: 'Color Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
    };

    usePatternStore.getState().savePattern(binaryPattern);
    usePatternStore.getState().savePattern(colorPattern);

    renderWithProviders(<BulbPatternPage />);

    const chooser = screen.getByLabelText(/saved patterns/i);

    // Check that Binary Pattern is an option
    expect(within(chooser).getByRole('option', { name: 'Binary Pattern' })).toBeInTheDocument();

    // Check that Color Pattern is NOT an option
    expect(within(chooser).queryByRole('option', { name: 'Color Pattern' })).not.toBeInTheDocument();
  });
});
