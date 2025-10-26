import userEvent from '@testing-library/user-event';
import { useState } from 'react';
import { describe, expect, it, vi } from 'vitest';

import type { ModePattern } from '@/app/models/mode';
import { renderWithProviders, screen } from '@/test-utils/render-with-providers';

import {
  SimpleRgbPatternPanel,
  type SimpleRgbPatternPanelProps,
} from './SimpleRgbPatternPanel';

const createPattern = (segments: { color: string; duration: number }[]): ModePattern => {
  let cursor = 0;

  const changeAt = segments.map(segment => {
    const entry = {
      ms: cursor,
      output: segment.color as ModePattern['changeAt'][number]['output'],
    };
    cursor += segment.duration;
    return entry;
  });

  return {
    name: 'test-pattern',
    duration: cursor,
    changeAt,
  };
};

const renderComponent = (props?: Partial<SimpleRgbPatternPanelProps>) =>
  renderWithProviders(
    <SimpleRgbPatternPanel
      onChange={vi.fn()}
      value={createPattern([])}
      {...props}
    />,
  );

describe('SimpleRgbPatternPanel', () => {
  it('shows empty preview when no steps are defined', () => {
    renderComponent({ value: createPattern([]) });

    expect(screen.getAllByText(/no colors have been added yet/i)).toHaveLength(2);
    const addButton = screen.getByRole('button', { name: /add color step/i });
    expect(addButton).toBeEnabled();
  const durationInput = screen.getByLabelText(/duration/i);
  expect(durationInput).toHaveValue(250);
  });

  it('emits an add-step action with the new segment when submitting the form', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();

    renderComponent({
      onChange: handleChange,
      value: createPattern([]),
    });

    const durationInput = screen.getByLabelText(/duration/i);
  await user.clear(durationInput);
  await user.type(durationInput, '200');
    await user.click(screen.getByRole('button', { name: /add color step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<SimpleRgbPatternPanelProps['onChange']>;

    expect(nextPattern.duration).toBe(200);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: '#ff7b00' as ModePattern['changeAt'][number]['output'] },
    ]);
    expect(action.type).toBe('add-step');
    if (action.type === 'add-step') {
      expect(action.step).toMatchObject({ color: '#ff7b00', durationMs: 200 });
    }
  });

  it('emits remove-step when removing a segment', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([{ color: '#112233', duration: 150 }]);

    renderComponent({
      onChange: handleChange,
      value: pattern,
    });

    await user.click(screen.getByRole('button', { name: /remove step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<SimpleRgbPatternPanelProps['onChange']>;

    expect(nextPattern.changeAt).toEqual([]);
    expect(nextPattern.duration).toBe(0);
    expect(action).toEqual({ type: 'remove-step', stepId: 'rgb-step-0-0' });
  });

  it('emits rename-pattern when updating the pattern name', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const Harness = () => {
      const [pattern, setPattern] = useState(createPattern([]));
      return (
        <SimpleRgbPatternPanel
          onChange={(nextPattern, action) => {
            setPattern(nextPattern);
            handleChange(nextPattern, action);
          }}
          value={pattern}
        />
      );
    };

    renderWithProviders(<Harness />);

    const nameInput = screen.getByLabelText(/pattern name/i);
    await user.clear(nameInput);
    await user.type(nameInput, 'Evening Breeze');

    expect(handleChange).toHaveBeenCalled();
    const lastCall = handleChange.mock.calls.at(-1) as Parameters<SimpleRgbPatternPanelProps['onChange']> | undefined;
    expect(lastCall).toBeDefined();
    if (!lastCall) {
      throw new Error('Expected handleChange to be called when renaming');
    }
    const [nextPattern, action] = lastCall;

    expect(nextPattern.name).toBe('Evening Breeze');
    expect(action).toEqual({ type: 'rename-pattern', name: 'Evening Breeze' });
  });

  it('moves a step when reordering', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([
      { color: '#101010', duration: 100 },
      { color: '#202020', duration: 200 },
    ]);

    renderComponent({
      onChange: handleChange,
      value: pattern,
    });

    const moveDownButtons = screen.getAllByRole('button', { name: /move down/i });
    await user.click(moveDownButtons[0]);

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<SimpleRgbPatternPanelProps['onChange']>;

    expect(nextPattern.duration).toBe(300);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: '#202020' as ModePattern['changeAt'][number]['output'] },
      { ms: 200, output: '#101010' as ModePattern['changeAt'][number]['output'] },
    ]);
    expect(action).toEqual({ type: 'move-step', fromIndex: 0, toIndex: 1 });
  });

  it('duplicates a step directly after the source', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([{ color: '#334455', duration: 300 }]);

    renderComponent({
      onChange: handleChange,
      value: pattern,
    });

    await user.click(screen.getByRole('button', { name: /duplicate step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<SimpleRgbPatternPanelProps['onChange']>;

    expect(nextPattern.duration).toBe(600);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: '#334455' as ModePattern['changeAt'][number]['output'] },
      { ms: 300, output: '#334455' as ModePattern['changeAt'][number]['output'] },
    ]);
    expect(action.type).toBe('duplicate-step');
    if (action.type === 'duplicate-step') {
      expect(action.sourceId).toBe('rgb-step-0-0');
      expect(action.newStep.color).toBe('#334455');
      expect(action.newStep.durationMs).toBe(300);
    }
  });

  it('ignores non-integer duration input characters', async () => {
    const user = userEvent.setup();
    renderComponent({ value: createPattern([]) });

    const durationInput = screen.getByLabelText(/duration/i);
    await user.clear(durationInput);
    await user.type(durationInput, '1.5');

    expect(durationInput).toHaveValue(1);
  });

  it('summarizes the total duration when steps exist', () => {
    renderComponent({
      value: createPattern([
        { color: '#000000', duration: 100 },
        { color: '#ffffff', duration: 400 },
      ]),
    });

    expect(screen.getByText(/total duration 500 ms/i)).toBeInTheDocument();
    const segments = screen.getAllByLabelText(/for \d+ ms/i);
    expect(segments).toHaveLength(2);
    const swatches = screen.getAllByTestId('rgb-step-color');
    expect(swatches).toHaveLength(2);
  });
});
