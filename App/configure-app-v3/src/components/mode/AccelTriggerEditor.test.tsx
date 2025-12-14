import { fireEvent, screen } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';

import { AccelTriggerEditor } from './AccelTriggerEditor';
import { hexColorSchema, type ModeAccelTrigger, type ModePattern } from '../../app/models/mode';
import { renderWithProviders } from '../../test-utils/render-with-providers';

const mockPatterns: ModePattern[] = [
  {
    type: 'simple',
    name: 'Binary Pattern',
    duration: 1000,
    changeAt: [{ ms: 0, output: 'high' }],
  },
  {
    type: 'simple',
    name: 'Color Pattern',
    duration: 1000,
    changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
  },
];

describe('AccelTriggerEditor', () => {
  it('renders empty state', () => {
    renderWithProviders(
      <AccelTriggerEditor triggers={[]} onChange={vi.fn()} patterns={mockPatterns} />,
    );
    expect(screen.getByText('No accelerometer triggers defined.')).toBeInTheDocument();
  });

  it('adds a trigger', () => {
    const onChange = vi.fn();
    renderWithProviders(
      <AccelTriggerEditor triggers={[]} onChange={onChange} patterns={mockPatterns} />,
    );

    fireEvent.click(screen.getByText('Add Trigger'));
    expect(onChange).toHaveBeenCalledWith([expect.objectContaining({ threshold: 1.5 })]);
  });

  it('renders triggers', () => {
    const triggers = [
      {
        threshold: 2.5,
        front: undefined,
        case: undefined,
      },
    ];
    renderWithProviders(
      <AccelTriggerEditor triggers={triggers} onChange={vi.fn()} patterns={mockPatterns} />,
    );

    expect(screen.getByDisplayValue('2.5')).toBeInTheDocument();
  });

  it('updates threshold', () => {
    const triggers = [
      {
        threshold: 2.5,
        front: undefined,
        case: undefined,
      },
    ];
    const onChange = vi.fn();
    renderWithProviders(
      <AccelTriggerEditor triggers={triggers} onChange={onChange} patterns={mockPatterns} />,
    );

    fireEvent.change(screen.getByDisplayValue('2.5'), { target: { value: '3.0' } });
    expect(onChange).toHaveBeenCalledWith([expect.objectContaining({ threshold: 3.0 })]);
  });

  it('removes trigger', () => {
    const triggers = [
      {
        threshold: 2.5,
        front: undefined,
        case: undefined,
      },
    ];
    const onChange = vi.fn();
    renderWithProviders(
      <AccelTriggerEditor triggers={triggers} onChange={onChange} patterns={mockPatterns} />,
    );

    fireEvent.click(screen.getByText('Delete'));
    expect(onChange).toHaveBeenCalledWith([]);
  });

  it('updates pattern override', () => {
    const triggers = [
      {
        threshold: 2.5,
        front: undefined,
        case: undefined,
      },
    ];
    const onChange = vi.fn();
    renderWithProviders(
      <AccelTriggerEditor triggers={triggers} onChange={onChange} patterns={mockPatterns} />,
    );

    // Find the first select (Front Override)
    const selects = screen.getAllByRole('combobox');
    fireEvent.change(selects[0], { target: { value: 'Binary Pattern' } });

    expect(onChange).toHaveBeenCalledTimes(1);
    const newTriggers = onChange.mock.calls[0][0] as ModeAccelTrigger[];
    expect(newTriggers).toHaveLength(1);
    expect(newTriggers[0].front?.pattern.name).toBe('Binary Pattern');
  });
});
