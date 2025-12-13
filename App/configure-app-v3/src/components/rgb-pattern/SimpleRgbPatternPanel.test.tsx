import userEvent from '@testing-library/user-event';
import { useState } from 'react';
import { describe, expect, it, vi } from 'vitest';

import type { SimplePattern } from '@/app/models/mode';
import { fireEvent, renderWithProviders, screen, waitFor } from '@/test-utils/render-with-providers';

import {
  SimpleRgbPatternPanel,
  type SimpleRgbPatternPanelProps,
} from './SimpleRgbPatternPanel';

const createPattern = (segments: { color: string; duration: number }[]): SimplePattern => {
  let cursor = 0;

  const changeAt = segments.map(segment => {
    const entry = {
      ms: cursor,
      output: segment.color as SimplePattern['changeAt'][number]['output'],
    };
    cursor += segment.duration;
    return entry;
  });

  return {
    type: 'simple',
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
    expect(screen.queryByLabelText(/duration/i)).not.toBeInTheDocument();
  });

  it('emits an add-step action with the new segment when confirming the modal', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();

    renderComponent({
      onChange: handleChange,
      value: createPattern([]),
    });

    await user.click(screen.getByRole('button', { name: /add color step/i }));

    const modalDurationInput = await screen.findByLabelText(/duration/i);
    await user.clear(modalDurationInput);
    await user.type(modalDurationInput, '200');

    await user.click(screen.getByRole('button', { name: /add step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<SimpleRgbPatternPanelProps['onChange']>;

    expect(nextPattern.duration).toBe(200);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: '#ff7b00' as SimplePattern['changeAt'][number]['output'] },
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

    // Select the segment first
    await user.click(screen.getByLabelText(/#112233 for 150 ms/i));

    // Now click remove
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

    // Select the first segment
    await user.click(screen.getByLabelText(/#101010 for 100 ms/i));

    const moveDownButton = screen.getByRole('button', { name: /move down/i });
    await user.click(moveDownButton);

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<SimpleRgbPatternPanelProps['onChange']>;

    expect(nextPattern.duration).toBe(300);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: '#202020' as SimplePattern['changeAt'][number]['output'] },
      { ms: 200, output: '#101010' as SimplePattern['changeAt'][number]['output'] },
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

    // Select the segment
    await user.click(screen.getByLabelText(/#334455 for 300 ms/i));

    await user.click(screen.getByRole('button', { name: /duplicate step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<SimpleRgbPatternPanelProps['onChange']>;

    expect(nextPattern.duration).toBe(600);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: '#334455' as SimplePattern['changeAt'][number]['output'] },
      { ms: 300, output: '#334455' as SimplePattern['changeAt'][number]['output'] },
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

    await user.click(screen.getByRole('button', { name: /add color step/i }));

    const durationInput = await screen.findByLabelText(/duration/i);
    await user.clear(durationInput);
    await user.type(durationInput, '1.5');

    expect(durationInput).toHaveValue(1);
  });

  it('updates an existing step when editing color and duration', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([
      { color: '#000000', duration: 100 },
      { color: '#ffffff', duration: 200 },
    ]);

    const Harness = () => {
      const [value, setValue] = useState(pattern);
      return (
        <SimpleRgbPatternPanel
          onChange={(nextPattern, action) => {
            setValue(nextPattern);
            handleChange(nextPattern, action);
          }}
          value={value}
        />
      );
    };

    renderWithProviders(<Harness />);

    // Select the second segment (index 1)
    await user.click(screen.getByLabelText(/#ffffff for 200 ms/i));

    const colorInput = screen.getByLabelText(/^color/i);
    // Use fireEvent for color input as userEvent.type doesn't work well with color inputs
    fireEvent.input(colorInput, { target: { value: '#123456' } });

    const durationInput = screen.getByLabelText(/duration/i);
    await user.clear(durationInput);
    await user.type(durationInput, '150');
    expect(durationInput).toHaveValue(150);

    const updateCalls = handleChange.mock.calls.filter((call): call is Parameters<SimpleRgbPatternPanelProps['onChange']> => {
      const [, action] = call as Parameters<SimpleRgbPatternPanelProps['onChange']>;
      return action.type === 'update-step';
    });

    const finalCall = updateCalls.at(-1);
    if (!finalCall) {
      throw new Error('Expected an update-step action when editing a step');
    }

    const [nextPattern, action] = finalCall;
    expect(nextPattern.duration).toBe(250);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: '#000000' as SimplePattern['changeAt'][number]['output'] },
      { ms: 100, output: '#123456' as SimplePattern['changeAt'][number]['output'] },
    ]);
    expect(action.type).toBe('update-step');
    if (action.type === 'update-step') {
      expect(action.step.color).toBe('#123456');
      expect(action.step.durationMs).toBe(150);
    }
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
  });

  it('closes the modal when cancelling', async () => {
    const user = userEvent.setup();
    renderComponent({ value: createPattern([]) });

    await user.click(screen.getByRole('button', { name: /add color step/i }));

    expect(await screen.findByRole('dialog')).toBeInTheDocument();

    await user.click(screen.getByRole('button', { name: /cancel/i }));

    await waitFor(() => {
      expect(screen.queryByRole('dialog')).not.toBeInTheDocument();
    });
  });
});
