import { fireEvent, screen } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';

import { type ModeAction, ModeEditor } from './ModeEditor';
import { hexColorSchema, type Mode, type ModePattern } from '../../app/models/mode';
import { renderWithProviders } from '../../test-utils/render-with-providers';

const mockPatterns: ModePattern[] = [
  {
    type: 'simple',
    name: 'Front Pattern',
    duration: 1000,
    changeAt: [{ ms: 0, output: 'high' }],
  },
  {
    type: 'simple',
    name: 'Case Pattern',
    duration: 1000,
    changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
  },
];

const defaultMode: Mode = {
  name: '',
  front: {
    pattern: {
      type: 'simple',
      name: 'default-front',
      duration: 1000,
      changeAt: [{ ms: 0, output: 'low' }],
    },
  },
  case: {
    pattern: {
      type: 'simple',
      name: 'default-case',
      duration: 1000,
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#000000') }],
    },
  },
};

describe('ModeEditor', () => {
  it('renders form with provided mode', () => {
    const mode = { ...defaultMode, name: 'Test Mode' };
    renderWithProviders(<ModeEditor mode={mode} onChange={vi.fn()} patterns={mockPatterns} />);
    expect(screen.getByDisplayValue('Test Mode')).toBeInTheDocument();
  });

  it('calls onChange when name changes', () => {
    const onChange = vi.fn();
    renderWithProviders(
      <ModeEditor mode={defaultMode} onChange={onChange} patterns={mockPatterns} />,
    );

    fireEvent.change(screen.getByPlaceholderText('Enter mode name'), {
      target: { value: 'New Name' },
    });

    expect(onChange).toHaveBeenCalledWith(
      expect.objectContaining({ name: 'New Name' }),
      expect.objectContaining({ type: 'update-name', name: 'New Name' }),
    );
  });

  it('calls onChange when pattern changes', () => {
    const onChange = vi.fn();
    renderWithProviders(
      <ModeEditor mode={defaultMode} onChange={onChange} patterns={mockPatterns} />,
    );

    const selects = screen.getAllByRole('combobox');
    // Front pattern
    fireEvent.change(selects[0], { target: { value: 'Front Pattern' } });

    expect(onChange).toHaveBeenCalledTimes(1);
    const [newMode, action] = onChange.mock.calls[0] as [Mode, ModeAction];

    expect(newMode.front?.pattern.name).toBe('Front Pattern');
    expect(action.type).toBe('update-front-pattern');
    if (action.type === 'update-front-pattern') {
      expect(action.pattern?.name).toBe('Front Pattern');
    }
  });

  it('calls onChange when pattern is cleared', () => {
    const onChange = vi.fn();
    renderWithProviders(
      <ModeEditor mode={defaultMode} onChange={onChange} patterns={mockPatterns} />,
    );

    const selects = screen.getAllByRole('combobox');
    // Front pattern
    fireEvent.change(selects[0], { target: { value: '' } });

    expect(onChange).toHaveBeenCalledTimes(1);
    const [newMode, action] = onChange.mock.calls[0] as [Mode, ModeAction];

    expect(newMode.front).toBeUndefined();
    expect(action.type).toBe('update-front-pattern');
    if (action.type === 'update-front-pattern') {
      expect(action.pattern).toBeUndefined();
    }
  });
});
