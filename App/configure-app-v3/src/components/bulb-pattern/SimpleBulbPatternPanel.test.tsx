import userEvent from '@testing-library/user-event';
import { useState } from 'react';
import { describe, expect, it, vi } from 'vitest';

import { SimpleBulbPatternPanel, type SimpleBulbPatternPanelProps } from './SimpleBulbPatternPanel';
import { type SimplePattern } from '../../app/models/mode';
import {
  renderWithProviders,
  screen,
  waitFor,
  within,
} from '../../test-utils/render-with-providers';

const createPattern = (segments: { output: 'high' | 'low'; duration: number }[]): SimplePattern => {
  let cursor = 0;

  const changeAt = segments.map(segment => {
    const entry = {
      ms: cursor,
      output: segment.output,
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

const renderComponent = (props?: Partial<SimpleBulbPatternPanelProps>) =>
  renderWithProviders(
    <SimpleBulbPatternPanel onChange={vi.fn()} value={createPattern([])} {...props} />,
  );

describe('SimpleBulbPatternPanel', () => {
  it('shows empty preview when no steps are defined', () => {
    renderComponent({ value: createPattern([]) });

    expect(screen.getAllByText(/no steps have been added yet/i)).toHaveLength(2);
    const addButton = screen.getByRole('button', { name: /add step/i });
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

    await user.click(screen.getByRole('button', { name: /add step/i }));

    const modalDurationInput = await screen.findByLabelText(/duration/i);
    await user.clear(modalDurationInput);
    await user.type(modalDurationInput, '200');

    // Default value is 'high'
    const dialog = screen.getByRole('dialog');
    await user.click(within(dialog).getByRole('button', { name: /add step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<
      SimpleBulbPatternPanelProps['onChange']
    >;

    expect(nextPattern.duration).toBe(200);
    expect(nextPattern.changeAt).toEqual([{ ms: 0, output: 'high' }]);
    expect(action.type).toBe('add-step');
    if (action.type === 'add-step') {
      expect(action.step).toMatchObject({ value: 'high', durationMs: 200 });
    }
  });

  it('emits remove-step when removing a segment', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([{ output: 'low', duration: 150 }]);

    renderComponent({
      onChange: handleChange,
      value: pattern,
    });

    // Select the segment first
    await user.click(screen.getByLabelText(/low for 150 ms/i));

    // Now click remove
    await user.click(screen.getByRole('button', { name: /remove step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<
      SimpleBulbPatternPanelProps['onChange']
    >;

    expect(nextPattern.changeAt).toEqual([]);
    expect(nextPattern.duration).toBe(0);
    expect(action.type).toBe('remove-step');
    if (action.type === 'remove-step') {
      expect(action.stepId).toMatch(/^step-/);
    }
  });

  it('emits rename-pattern when updating the pattern name', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const Harness = () => {
      const [pattern, setPattern] = useState(createPattern([]));
      return (
        <SimpleBulbPatternPanel
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
    await user.type(nameInput, 'Blinking Light');

    expect(handleChange).toHaveBeenCalled();
    const lastCall = handleChange.mock.calls.at(-1) as
      | Parameters<SimpleBulbPatternPanelProps['onChange']>
      | undefined;
    expect(lastCall).toBeDefined();
    if (!lastCall) throw new Error('Expected call');
    const [nextPattern, action] = lastCall;

    expect(nextPattern.name).toBe('Blinking Light');
    expect(action).toEqual({ type: 'rename-pattern', name: 'Blinking Light' });
  });

  it('moves a step when reordering', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([
      { output: 'high', duration: 100 },
      { output: 'low', duration: 200 },
    ]);

    renderComponent({
      onChange: handleChange,
      value: pattern,
    });

    // Select the first segment
    await user.click(screen.getByLabelText(/high for 100 ms/i));

    const moveDownButton = screen.getByRole('button', { name: /move down/i });
    await user.click(moveDownButton);

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<
      SimpleBulbPatternPanelProps['onChange']
    >;

    expect(nextPattern.duration).toBe(300);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: 'low' },
      { ms: 200, output: 'high' },
    ]);
    expect(action).toEqual({ type: 'move-step', fromIndex: 0, toIndex: 1 });
  });

  it('duplicates a step directly after the source', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([{ output: 'high', duration: 300 }]);

    renderComponent({
      onChange: handleChange,
      value: pattern,
    });

    // Select the segment
    await user.click(screen.getByLabelText(/high for 300 ms/i));

    await user.click(screen.getByRole('button', { name: /duplicate step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<
      SimpleBulbPatternPanelProps['onChange']
    >;

    expect(nextPattern.duration).toBe(600);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: 'high' },
      { ms: 300, output: 'high' },
    ]);
    expect(action.type).toBe('duplicate-step');
    if (action.type === 'duplicate-step') {
      expect(action.newStep.value).toBe('high');
      expect(action.newStep.durationMs).toBe(300);
    }
  });

  it('updates an existing step when editing state and duration', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([
      { output: 'low', duration: 100 },
      { output: 'high', duration: 200 },
    ]);

    const Harness = () => {
      const [value, setValue] = useState(pattern);
      return (
        <SimpleBulbPatternPanel
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
    await user.click(screen.getByLabelText(/high for 200 ms/i));

    // Toggle the switch
    const switchButton = screen.getByRole('switch', { name: /state/i });
    await user.click(switchButton);

    const durationInput = screen.getByLabelText(/duration/i);
    await user.clear(durationInput);
    await user.type(durationInput, '150');

    const updateCalls = handleChange.mock.calls.filter(
      (call): call is Parameters<SimpleBulbPatternPanelProps['onChange']> => {
        const [, action] = call as Parameters<SimpleBulbPatternPanelProps['onChange']>;
        return action.type === 'update-step';
      },
    );

    const finalCall = updateCalls.at(-1);
    expect(finalCall).toBeDefined();
    if (!finalCall) throw new Error('Expected call');

    const [nextPattern, action] = finalCall;
    expect(nextPattern.duration).toBe(250);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: 'low' },
      { ms: 100, output: 'low' },
    ]);
    expect(action.type).toBe('update-step');
    if (action.type === 'update-step') {
      expect(action.step.value).toBe('low');
      expect(action.step.durationMs).toBe(150);
    }
  });

  it('summarizes the total duration when steps exist', () => {
    renderComponent({
      value: createPattern([
        { output: 'low', duration: 100 },
        { output: 'high', duration: 400 },
      ]),
    });

    expect(screen.getByText(/total duration 500 ms/i)).toBeInTheDocument();
    const segments = screen.getAllByLabelText(/for \d+ ms/i);
    expect(segments).toHaveLength(2);
  });

  it('ignores non-integer duration input characters', async () => {
    const user = userEvent.setup();
    renderComponent({ value: createPattern([]) });

    await user.click(screen.getByRole('button', { name: /add step/i }));

    const durationInput = await screen.findByLabelText(/duration/i);
    await user.clear(durationInput);
    await user.type(durationInput, '1.5');

    expect(durationInput).toHaveValue(1);
  });

  it('closes the modal when cancelling', async () => {
    const user = userEvent.setup();
    renderComponent({ value: createPattern([]) });

    await user.click(screen.getByRole('button', { name: /add step/i }));

    expect(await screen.findByRole('dialog')).toBeInTheDocument();

    await user.click(screen.getByRole('button', { name: /cancel/i }));

    await waitFor(() => {
      expect(screen.queryByRole('dialog')).not.toBeInTheDocument();
    });
  });

  it('allows setting duration to 0 to trigger validation errors', async () => {
    const onChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([{ output: 'high', duration: 100 }]);

    renderWithProviders(<SimpleBulbPatternPanel onChange={onChange} value={pattern} />);

    // Select the segment
    await user.click(screen.getByRole('button', { name: /high for 100 ms/i }));

    // Find duration input
    const durationInput = screen.getByRole('spinbutton', { name: /duration/i });

    // Change to 0
    await user.clear(durationInput);
    await user.type(durationInput, '0');

    // Check if onChange was called with 0 duration
    expect(onChange).toHaveBeenLastCalledWith(
      expect.objectContaining({
        duration: 0,
      }),
      expect.objectContaining({
        type: 'update-step',
      }),
    );
  });

  it('renders a step even if its duration is 0 (single step case)', () => {
    const pattern: SimplePattern = {
      type: 'simple',
      name: 'test',
      duration: 0,
      changeAt: [{ ms: 0, output: 'high' }],
    };

    renderWithProviders(<SimpleBulbPatternPanel onChange={vi.fn()} value={pattern} />);

    expect(screen.getByRole('button', { name: /high for 0 ms/i })).toBeInTheDocument();
  });

  it('triggers validation error when duration is empty', async () => {
    const onChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([{ output: 'high', duration: 100 }]);

    renderWithProviders(<SimpleBulbPatternPanel onChange={onChange} value={pattern} />);

    // Select the segment
    await user.click(screen.getByRole('button', { name: /high for 100 ms/i }));

    // Find duration input
    const durationInput = screen.getByRole('spinbutton', { name: /duration/i });

    // Clear input
    await user.clear(durationInput);

    // Check if onChange was called with 0 duration (NaN treated as 0 to prevent cascading)
    expect(onChange).toHaveBeenLastCalledWith(
      expect.objectContaining({
        duration: 0,
      }),
      expect.objectContaining({
        type: 'update-step',
      }),
    );
  });

  it('preserves subsequent steps when clearing duration of the first step', async () => {
    const onChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([
      { output: 'high', duration: 100 },
      { output: 'low', duration: 100 },
    ]);

    renderWithProviders(<SimpleBulbPatternPanel onChange={onChange} value={pattern} />);

    // Select the first step
    await user.click(screen.getByRole('button', { name: /high for 100 ms/i }));

    // Find the duration input
    const durationInput = screen.getByRole('spinbutton', { name: /duration/i });

    // Clear the input
    await user.clear(durationInput);

    // Verify the second step still exists and has valid duration in the UI
    expect(screen.getByRole('button', { name: /low for 100 ms/i })).toBeInTheDocument();
  });
});
