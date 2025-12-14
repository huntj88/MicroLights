import { fireEvent, screen } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';

import { PatternSelector } from './PatternSelector';
import { hexColorSchema, type ModePattern } from '../../app/models/mode';
import { renderWithProviders } from '../../test-utils/render-with-providers';

const mockPatterns: ModePattern[] = [
  {
    type: 'simple',
    name: 'Pattern 1',
    duration: 1000,
    changeAt: [{ ms: 0, output: 'high' }],
  },
  {
    type: 'simple',
    name: 'Pattern 2',
    duration: 1000,
    changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
  },
];

describe('PatternSelector', () => {
  it('renders options from props', () => {
    const onChange = vi.fn();
    renderWithProviders(
      <PatternSelector
        label="Select Pattern"
        onChange={onChange}
        value=""
        patterns={mockPatterns}
      />,
    );

    expect(screen.getByText('Pattern 1')).toBeInTheDocument();
    expect(screen.getByText('Pattern 2')).toBeInTheDocument();
  });

  it('renders only provided patterns', () => {
    const onChange = vi.fn();
    const filteredPatterns = mockPatterns.filter(p => p.name === 'Pattern 1');
    renderWithProviders(
      <PatternSelector
        label="Select Pattern"
        onChange={onChange}
        value=""
        patterns={filteredPatterns}
      />,
    );

    expect(screen.getByText('Pattern 1')).toBeInTheDocument();
    expect(screen.queryByText('Pattern 2')).not.toBeInTheDocument();
  });

  it('calls onChange when selected', () => {
    const onChange = vi.fn();
    renderWithProviders(
      <PatternSelector
        label="Select Pattern"
        onChange={onChange}
        value=""
        patterns={mockPatterns}
      />,
    );

    fireEvent.change(screen.getByRole('combobox'), { target: { value: 'Pattern 1' } });
    expect(onChange).toHaveBeenCalledWith('Pattern 1');
  });
});
