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

  it('shows validation errors for empty mode', () => {
    renderWithProviders(<ModePage />);

    // Initially empty, should show errors
    expect(screen.getByText('Mode name cannot be empty.')).toBeInTheDocument();
    expect(
      screen.getByText('At least one pattern (Front or Case) is required.'),
    ).toBeInTheDocument();

    // Save button should be disabled
    expect(screen.getByText('Save Mode')).toBeDisabled();
  });

  it('clears validation errors when mode becomes valid', () => {
    usePatternStore.setState({
      patterns: [
        {
          type: 'simple',
          name: 'Front Pattern',
          duration: 1000,
          changeAt: [{ ms: 0, output: 'high' }],
        },
      ],
    });

    renderWithProviders(<ModePage />);

    // Fix name error
    fireEvent.change(screen.getByPlaceholderText('Enter mode name'), {
      target: { value: 'Valid Mode' },
    });
    expect(screen.queryByText('Mode name cannot be empty.')).not.toBeInTheDocument();

    // Fix pattern error
    const selects = screen.getAllByRole('combobox');
    fireEvent.change(selects[1], { target: { value: 'Front Pattern' } });
    expect(
      screen.queryByText('At least one pattern (Front or Case) is required.'),
    ).not.toBeInTheDocument();

    // Save button should be enabled
    expect(screen.getByText('Save Mode')).toBeEnabled();
  });

  it('saves a mode with triggers', () => {
    usePatternStore.setState({
      patterns: [
        {
          type: 'simple',
          name: 'Front Pattern',
          duration: 1000,
          changeAt: [{ ms: 0, output: 'high' }],
        },
      ],
    });

    renderWithProviders(<ModePage />);

    fireEvent.change(screen.getByPlaceholderText('Enter mode name'), {
      target: { value: 'Trigger Mode' },
    });

    const selects = screen.getAllByRole('combobox');
    fireEvent.change(selects[1], { target: { value: 'Front Pattern' } });

    fireEvent.click(screen.getByText('Add Trigger'));

    // Trigger added, but invalid (needs pattern).
    // Find trigger pattern selector.
    // Selects: 0=saved, 1=front, 2=case, 3=trigger-front, 4=trigger-case
    const triggerSelects = screen.getAllByRole('combobox');
    fireEvent.change(triggerSelects[3], { target: { value: 'Front Pattern' } });

    fireEvent.click(screen.getByText('Save Mode'));

    const modes = useModeStore.getState().modes;
    expect(modes).toHaveLength(1);
    expect(modes[0].accel?.triggers).toHaveLength(1);
    expect(modes[0].accel?.triggers[0].front?.pattern.name).toBe('Front Pattern');
  });

  it('validates triggers', () => {
    usePatternStore.setState({
      patterns: [
        {
          type: 'simple',
          name: 'Front Pattern',
          duration: 1000,
          changeAt: [{ ms: 0, output: 'high' }],
        },
      ],
    });

    renderWithProviders(<ModePage />);

    // Make mode valid first
    fireEvent.change(screen.getByPlaceholderText('Enter mode name'), {
      target: { value: 'Trigger Mode' },
    });
    const selects = screen.getAllByRole('combobox');
    fireEvent.change(selects[1], { target: { value: 'Front Pattern' } });

    // Add trigger - initially invalid
    fireEvent.click(screen.getByText('Add Trigger'));

    expect(
      screen.getByText('Accelerometer triggers must configure at least one LED component.'),
    ).toBeInTheDocument();
    expect(screen.getByText('Save Mode')).toBeDisabled();

    // Fix trigger
    const triggerSelects = screen.getAllByRole('combobox');
    fireEvent.change(triggerSelects[3], { target: { value: 'Front Pattern' } });

    expect(
      screen.queryByText('Accelerometer triggers must configure at least one LED component.'),
    ).not.toBeInTheDocument();
    expect(screen.getByText('Save Mode')).toBeEnabled();
  });
});
