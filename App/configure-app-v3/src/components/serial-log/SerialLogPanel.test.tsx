import userEvent from '@testing-library/user-event';
import { useState } from 'react';
import { beforeEach, describe, expect, it, vi } from 'vitest';

import { renderWithProviders, screen } from '@/test-utils/render-with-providers';

import { SerialLogPanel } from './SerialLogPanel';
import type { SerialLogAction, SerialLogState } from './SerialLogPanel';

describe('SerialLogPanel', () => {
  const baseState: SerialLogState = {
    autoscroll: true,
    entries: [],
    pendingPayload: '',
  };

  type ChangeHandler = (newState: SerialLogState, action: SerialLogAction) => void;
  interface ControlledProps {
    onChange: ChangeHandler;
    initialState?: SerialLogState;
  }

  const ControlledSerialLogPanel = ({ onChange, initialState = baseState }: ControlledProps) => {
    const [state, setState] = useState<SerialLogState>(initialState);

    const handleChange: ChangeHandler = (newState, action) => {
      setState(newState);
      onChange(newState, action);
    };

    return <SerialLogPanel onChange={handleChange} value={state} />;
  };

  beforeEach(() => {
    window.localStorage.clear();
  });

  it('renders empty state when there are no messages', () => {
    renderWithProviders(<SerialLogPanel onChange={vi.fn()} value={baseState} />);

    expect(screen.getByText('serialLog.empty')).toBeInTheDocument();
  });

  it('emits updated payload when typing', async () => {
    const user = userEvent.setup();
    const handleChange = vi.fn<ChangeHandler>();

    renderWithProviders(<ControlledSerialLogPanel onChange={handleChange} />);

    await user.type(screen.getByLabelText(/payload/i), 'test');

    const lastCall = handleChange.mock.calls.at(-1);
    expect(lastCall).toBeDefined();
    if (!lastCall) {
      throw new Error('Expected payload update callback');
    }

    expect(lastCall[0]).toMatchObject({ pendingPayload: 'test' });
    expect(lastCall[1]).toEqual({
      type: 'update-payload',
      value: 'test',
    });
  });

  it('toggles autoscroll', async () => {
    const user = userEvent.setup();
    const handleChange = vi.fn<ChangeHandler>();

    renderWithProviders(<SerialLogPanel onChange={handleChange} value={baseState} />);

    await user.click(screen.getByRole('button', { name: /autoscroll/i }));

    expect(handleChange).toHaveBeenCalledWith(expect.objectContaining({ autoscroll: false }), {
      type: 'toggle-autoscroll',
      autoscroll: false,
    });
  });

  it('submits payload and clears input', async () => {
    const user = userEvent.setup();
    const handleChange = vi.fn<ChangeHandler>();

    renderWithProviders(
      <SerialLogPanel onChange={handleChange} value={{ ...baseState, pendingPayload: 'ping' }} />,
    );

    await user.click(screen.getByRole('button', { name: 'serialLog.actions.send' }));

    expect(handleChange).toHaveBeenCalledWith(
      expect.objectContaining({
        pendingPayload: '',
      }),
      expect.objectContaining({
        type: 'submit',
      }),
    );

    const submitCall = handleChange.mock.calls.find(([, action]) => action.type === 'submit');
    expect(submitCall).toBeDefined();
    if (!submitCall) {
      throw new Error('Expected submit callback to be invoked');
    }
    const [newState] = submitCall;
    expect(newState.entries).toHaveLength(1);
    expect(newState.entries[0]).toMatchObject({
      payload: 'ping',
      direction: 'outbound',
    });
  });

  it('clears entries when requested', async () => {
    const user = userEvent.setup();
    const handleChange = vi.fn<ChangeHandler>();
    const populatedState: SerialLogState = {
      autoscroll: true,
      pendingPayload: '',
      entries: [
        {
          id: '1',
          timestamp: new Date().toISOString(),
          direction: 'inbound',
          payload: 'example',
        },
      ],
    };

    renderWithProviders(<SerialLogPanel onChange={handleChange} value={populatedState} />);

    await user.click(screen.getByRole('button', { name: 'serialLog.actions.clear' }));

    expect(handleChange).toHaveBeenCalledWith(expect.objectContaining({ entries: [] }), {
      type: 'clear',
    });
  });

  it('renders incoming messages', () => {
    const entries: SerialLogState['entries'] = [
      {
        id: '1',
        timestamp: new Date().toISOString(),
        direction: 'inbound',
        payload: 'System ready',
      },
    ];

    renderWithProviders(<SerialLogPanel onChange={vi.fn()} value={{ ...baseState, entries }} />);

    expect(screen.getByText('System ready')).toBeInTheDocument();
    expect(screen.getByText(/serialLog.direction.inbound/)).toBeInTheDocument();
  });

  it('autoscrolls to bottom when new message arrives', () => {
    const { rerender } = renderWithProviders(
      <SerialLogPanel onChange={vi.fn()} value={baseState} />,
    );

    const viewport = screen.getByRole('log');
    // Mock scrollTo which is not implemented in JSDOM
    viewport.scrollTo = vi.fn();
    // Mock scrollHeight
    Object.defineProperty(viewport, 'scrollHeight', { value: 500, configurable: true });

    const entries: SerialLogState['entries'] = [
      {
        id: '1',
        timestamp: new Date().toISOString(),
        direction: 'inbound',
        payload: 'New data',
      },
    ];

    rerender(<SerialLogPanel onChange={vi.fn()} value={{ ...baseState, entries }} />);

    // eslint-disable-next-line @typescript-eslint/unbound-method
    expect(viewport.scrollTo).toHaveBeenCalledWith({
      top: 500,
      behavior: 'smooth',
    });
  });

  it('does not autoscroll when disabled', () => {
    const { rerender } = renderWithProviders(
      <SerialLogPanel onChange={vi.fn()} value={{ ...baseState, autoscroll: false }} />,
    );

    const viewport = screen.getByRole('log');
    viewport.scrollTo = vi.fn();
    Object.defineProperty(viewport, 'scrollHeight', { value: 500, configurable: true });

    const entries: SerialLogState['entries'] = [
      {
        id: '1',
        timestamp: new Date().toISOString(),
        direction: 'inbound',
        payload: 'New data',
      },
    ];

    rerender(
      <SerialLogPanel onChange={vi.fn()} value={{ ...baseState, autoscroll: false, entries }} />,
    );

    // eslint-disable-next-line @typescript-eslint/unbound-method
    expect(viewport.scrollTo).not.toHaveBeenCalled();
  });
});
