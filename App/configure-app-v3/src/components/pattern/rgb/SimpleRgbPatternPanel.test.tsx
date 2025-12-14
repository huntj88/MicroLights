import userEvent from '@testing-library/user-event';
import { useState } from 'react';
import { describe, expect, it, vi } from 'vitest';

import { hexColorSchema, type SimplePattern } from '@/app/models/mode';
import {
  fireEvent,
  renderWithProviders,
  screen,
  waitFor,
  within,
} from '@/test-utils/render-with-providers';

import { SimpleRgbPatternPanel, type SimpleRgbPatternPanelProps } from './SimpleRgbPatternPanel';

const createPattern = (segments: { color: string; duration: number }[]): SimplePattern => {
  let cursor = 0;

  const changeAt = segments.map(segment => {
    const entry = {
      ms: cursor,
      output: hexColorSchema.parse(segment.color),
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
    <SimpleRgbPatternPanel onChange={vi.fn()} value={createPattern([])} {...props} />,
  );

describe('SimpleRgbPatternPanel', () => {
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

    const dialog = screen.getByRole('dialog');
    await user.click(within(dialog).getByRole('button', { name: /add step/i }));

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<
      SimpleRgbPatternPanelProps['onChange']
    >;

    expect(nextPattern.duration).toBe(200);
    expect(nextPattern.changeAt).toEqual([{ ms: 0, output: hexColorSchema.parse('#ff7b00') }]);
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
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<
      SimpleRgbPatternPanelProps['onChange']
    >;

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
    const lastCall = handleChange.mock.calls.at(-1) as
      | Parameters<SimpleRgbPatternPanelProps['onChange']>
      | undefined;
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
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<
      SimpleRgbPatternPanelProps['onChange']
    >;

    expect(nextPattern.duration).toBe(300);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: hexColorSchema.parse('#202020') },
      { ms: 200, output: hexColorSchema.parse('#101010') },
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
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<
      SimpleRgbPatternPanelProps['onChange']
    >;

    expect(nextPattern.duration).toBe(600);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: hexColorSchema.parse('#334455') },
      { ms: 300, output: hexColorSchema.parse('#334455') },
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

    await user.click(screen.getByRole('button', { name: /add step/i }));

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

    const updateCalls = handleChange.mock.calls.filter(
      (call): call is Parameters<SimpleRgbPatternPanelProps['onChange']> => {
        const [, action] = call as Parameters<SimpleRgbPatternPanelProps['onChange']>;
        return action.type === 'update-step';
      },
    );

    const finalCall = updateCalls.at(-1);
    if (!finalCall) {
      throw new Error('Expected an update-step action when editing a step');
    }

    const [nextPattern, action] = finalCall;
    expect(nextPattern.duration).toBe(250);
    expect(nextPattern.changeAt).toEqual([
      { ms: 0, output: hexColorSchema.parse('#000000') },
      { ms: 100, output: hexColorSchema.parse('#123456') },
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
    const pattern = createPattern([{ color: '#ff0000', duration: 100 }]);

    renderWithProviders(<SimpleRgbPatternPanel onChange={onChange} value={pattern} />);

    // Select the segment
    await user.click(screen.getByRole('button', { name: /#ff0000 for 100 ms/i }));

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
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
    };

    renderWithProviders(<SimpleRgbPatternPanel onChange={vi.fn()} value={pattern} />);

    expect(screen.getByRole('button', { name: /#ffffff for 0 ms/i })).toBeInTheDocument();
  });

  it('triggers validation error when duration is empty', async () => {
    const onChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([{ color: '#ff0000', duration: 100 }]);

    renderWithProviders(<SimpleRgbPatternPanel onChange={onChange} value={pattern} />);

    // Select the segment
    await user.click(screen.getByRole('button', { name: /#ff0000 for 100 ms/i }));

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
      { color: '#ff0000', duration: 100 },
      { color: '#00ff00', duration: 100 },
    ]);

    renderWithProviders(<SimpleRgbPatternPanel onChange={onChange} value={pattern} />);

    // Select the first step
    await user.click(screen.getByRole('button', { name: /#ff0000 for 100 ms/i }));

    // Find the duration input
    const durationInput = screen.getByRole('spinbutton', { name: /duration/i });

    // Clear the input
    await user.clear(durationInput);

    // Verify the second step still exists and has valid duration in the UI
    expect(screen.getByRole('button', { name: /#00ff00 for 100 ms/i })).toBeInTheDocument();
  });
});
