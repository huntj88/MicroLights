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

    expect(screen.getAllByText('patternEditor.preview.empty')).toHaveLength(2);
    const addButton = screen.getByRole('button', { name: 'patternEditor.form.addButton' });
    expect(addButton).toBeEnabled();
    expect(screen.queryByLabelText(/^patternEditor.form.durationLabel/)).not.toBeInTheDocument();
  });

  it('shows black swatch when pattern is empty', () => {
    renderComponent({ value: createPattern([]) });
    const swatch = screen.getByTestId('current-color-swatch');
    // hex #000000 is rgb(0, 0, 0)
    expect(swatch).toHaveStyle({ backgroundColor: 'rgb(0, 0, 0)' });
  });

  it('toggles playback state when clicking play/pause', async () => {
    const user = userEvent.setup();
    renderComponent({
      value: createPattern([{ color: '#ff0000', duration: 1000 }]),
    });

    const playButton = screen.getByRole('button', { name: 'patternEditor.controls.play' });
    await user.click(playButton);

    expect(screen.getByRole('button', { name: 'patternEditor.controls.pause' })).toBeInTheDocument();
    expect(screen.queryByRole('button', { name: 'patternEditor.controls.play' })).not.toBeInTheDocument();

    const pauseButton = screen.getByRole('button', { name: 'patternEditor.controls.pause' });
    await user.click(pauseButton);

    expect(screen.getByRole('button', { name: 'patternEditor.controls.play' })).toBeInTheDocument();
  });

  it('resets playback when clicking stop', async () => {
    const user = userEvent.setup();
    renderComponent({
      value: createPattern([{ color: '#ff0000', duration: 1000 }]),
    });

    const playButton = screen.getByRole('button', { name: 'patternEditor.controls.play' });
    await user.click(playButton);
    expect(screen.getByRole('button', { name: 'patternEditor.controls.pause' })).toBeInTheDocument();

    const stopButton = screen.getByRole('button', { name: 'patternEditor.controls.stop' });
    await user.click(stopButton);

    expect(screen.getByRole('button', { name: 'patternEditor.controls.play' })).toBeInTheDocument();
  });

  it('emits an add-step action with the new segment when confirming the modal', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();

    renderComponent({
      onChange: handleChange,
      value: createPattern([]),
    });

    await user.click(screen.getByRole('button', { name: 'patternEditor.form.addButton' }));

    const modalDurationInput = await screen.findByLabelText(/^patternEditor.form.durationLabel/);
    await user.clear(modalDurationInput);
    await user.type(modalDurationInput, '200');

    const dialog = screen.getByRole('dialog');
    await user.click(within(dialog).getByRole('button', { name: 'patternEditor.addModal.confirm' }));

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
    await user.click(screen.getByLabelText('patternEditor.preview.segmentLabel'));

    // Now click remove
    await user.click(screen.getByRole('button', { name: 'patternEditor.steps.remove' }));

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

    const nameInput = screen.getByLabelText(/^patternEditor.form.nameLabel/);
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
    const segments = screen.getAllByLabelText('patternEditor.preview.segmentLabel');
    await user.click(segments[0]);

    const moveDownButton = screen.getByRole('button', { name: /patternEditor.steps.moveDown/ });
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
    await user.click(screen.getByLabelText('patternEditor.preview.segmentLabel'));

    await user.click(screen.getByRole('button', { name: 'patternEditor.steps.duplicate' }));

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

    await user.click(screen.getByRole('button', { name: 'patternEditor.form.addButton' }));

    const durationInput = await screen.findByLabelText(/^patternEditor.form.durationLabel/);
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
    const segments = screen.getAllByLabelText('patternEditor.preview.segmentLabel');
    await user.click(segments[1]);

    const colorInput = screen.getByLabelText(/^rgbPattern.simple.form.colorLabel/);
    // Use fireEvent for color input as userEvent.type doesn't work well with color inputs
    fireEvent.input(colorInput, { target: { value: '#123456' } });

    const durationInput = screen.getByLabelText(/^patternEditor.form.durationLabel/);
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

    expect(screen.getByText('patternEditor.preview.summary')).toBeInTheDocument();
    const segments = screen.getAllByLabelText('patternEditor.preview.segmentLabel');
    expect(segments).toHaveLength(2);
  });

  it('closes the modal when cancelling', async () => {
    const user = userEvent.setup();
    renderComponent({ value: createPattern([]) });

    await user.click(screen.getByRole('button', { name: 'patternEditor.form.addButton' }));

    expect(await screen.findByRole('dialog')).toBeInTheDocument();

    await user.click(screen.getByRole('button', { name: 'patternEditor.addModal.cancel' }));

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
    await user.click(screen.getByRole('button', { name: 'patternEditor.preview.segmentLabel' }));

    // Find duration input
    const durationInput = screen.getByRole('spinbutton', { name: /^patternEditor.form.durationLabel/ });

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

    expect(screen.getByRole('button', { name: 'patternEditor.preview.segmentLabel' })).toBeInTheDocument();
  });

  it('triggers validation error when duration is empty', async () => {
    const onChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createPattern([{ color: '#ff0000', duration: 100 }]);

    renderWithProviders(<SimpleRgbPatternPanel onChange={onChange} value={pattern} />);

    // Select the segment
    await user.click(screen.getByRole('button', { name: 'patternEditor.preview.segmentLabel' }));

    // Find duration input
    const durationInput = screen.getByRole('spinbutton', { name: /^patternEditor.form.durationLabel/ });

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
    const segments = screen.getAllByLabelText('patternEditor.preview.segmentLabel');
    await user.click(segments[0]);

    // Find the duration input
    const durationInput = screen.getByRole('spinbutton', { name: /^patternEditor.form.durationLabel/ });

    // Clear the input
    await user.clear(durationInput);

    // Verify the second step still exists and has valid duration in the UI
    // Note: The second step will also have the same label 'patternEditor.preview.segmentLabel'
    // We can check that we still have 2 segments
    expect(screen.getAllByLabelText('patternEditor.preview.segmentLabel')).toHaveLength(2);
  });
});
