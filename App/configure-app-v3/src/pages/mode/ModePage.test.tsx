import { fireEvent, screen } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';

import { hexColorSchema } from '@/app/models/mode';

import { ModePage } from './ModePage';
import { useModeStore } from '../../app/providers/mode-store';
import { usePatternStore } from '../../app/providers/pattern-store';
import { renderWithProviders } from '../../test-utils/render-with-providers';

describe('ModePage', () => {
  it('renders', () => {
    renderWithProviders(<ModePage />);
    expect(screen.getByText('Mode Composer')).toBeInTheDocument();
  });

  it('saves a new mode', () => {
    usePatternStore.setState({
      patterns: [
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
      ],
    });

    renderWithProviders(<ModePage />);

    fireEvent.change(screen.getByPlaceholderText('Enter mode name'), {
      target: { value: 'New Mode' },
    });

    const selects = screen.getAllByRole('combobox');
    // 0 is saved modes, 1 is front pattern, 2 is case pattern
    fireEvent.change(selects[1], { target: { value: 'Front Pattern' } });
    fireEvent.change(selects[2], { target: { value: 'Case Pattern' } });

    fireEvent.click(screen.getByText('Save Mode'));

    const modes = useModeStore.getState().modes;
    expect(modes).toHaveLength(1);
    expect(modes[0].name).toBe('New Mode');
  });

  it('loads an existing mode', () => {
    usePatternStore.setState({
      patterns: [
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
      ],
    });

    useModeStore.setState({
      modes: [
        {
          name: 'Existing Mode',
          front: {
            pattern: {
              type: 'simple',
              name: 'Front Pattern',
              duration: 1000,
              changeAt: [{ ms: 0, output: 'high' }],
            },
          },
          case: {
            pattern: {
              type: 'simple',
              name: 'Case Pattern',
              duration: 1000,
              changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
            },
          },
        },
      ],
    });

    renderWithProviders(<ModePage />);

    const savedModesSelect = screen.getAllByRole('combobox')[0];
    fireEvent.change(savedModesSelect, { target: { value: 'Existing Mode' } });

    expect(screen.getByPlaceholderText('Enter mode name')).toHaveValue('Existing Mode');
  });

  it('deletes a mode', () => {
    // Mock confirm
    vi.spyOn(window, 'confirm').mockImplementation(() => true);

    useModeStore.setState({
      modes: [
        {
          name: 'Mode to Delete',
          front: {
            pattern: {
              type: 'simple',
              name: 'p1',
              duration: 1000,
              changeAt: [],
            },
          },
          case: {
            pattern: {
              type: 'simple',
              name: 'p2',
              duration: 1000,
              changeAt: [],
            },
          },
        },
      ],
    });

    renderWithProviders(<ModePage />);

    const savedModesSelect = screen.getAllByRole('combobox')[0];
    fireEvent.change(savedModesSelect, { target: { value: 'Mode to Delete' } });

    fireEvent.click(screen.getByText('Delete Mode'));

    const modes = useModeStore.getState().modes;
    expect(modes).toHaveLength(0);
  });
});
