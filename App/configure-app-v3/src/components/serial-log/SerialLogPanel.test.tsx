import userEvent from '@testing-library/user-event';
import { useState } from 'react';
import { beforeEach, describe, expect, it, vi } from 'vitest';

import { renderWithProviders, screen } from '@/test-utils/render-with-providers';

import { SerialLogPanel } from './SerialLogPanel';
import type { SerialLogAction, SerialLogState } from './SerialLogPanel';

describe('SerialLogPanel', () => {
  const baseState: SerialLogState = {
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

  it('renders "View Charger Status" button for charger data logs', () => {
    const chargerState: SerialLogState = {
      entries: [
        {
          id: '1',
          timestamp: new Date().toISOString(),
          direction: 'inbound',
          payload: JSON.stringify({
            chargectrl0: '00101100',
            stat0: '00000000',
            mask_id: '11000000',
          }),
        },
      ],
      pendingPayload: '',
    };

    renderWithProviders(<SerialLogPanel onChange={vi.fn()} value={chargerState} />);

    expect(screen.getByText('serialLog.actions.viewChargerStatus')).toBeInTheDocument();
  });

  it('renders "Import Mode" button for mode data logs', () => {
    const modeState: SerialLogState = {
      entries: [
        {
          id: '1',
          timestamp: new Date().toISOString(),
          direction: 'inbound',
          payload: JSON.stringify({
            command: 'writeMode',
            index: 0,
            mode: {
              name: 'test mode',
              front: {
                pattern: {
                  type: 'simple',
                  name: 'test',
                  duration: 100,
                  changeAt: [{ ms: 0, output: 'high' }],
                },
              },
            },
          }),
        },
      ],
      pendingPayload: '',
    };

    renderWithProviders(<SerialLogPanel onChange={vi.fn()} value={modeState} />);

    expect(screen.getByText('serialLog.actions.importMode')).toBeInTheDocument();
  });

  it('opens ChargerStatusModal when "View Charger Status" is clicked', async () => {
    const user = userEvent.setup();
    const chargerState: SerialLogState = {
      entries: [
        {
          id: '1',
          timestamp: new Date().toISOString(),
          direction: 'inbound',
          payload: JSON.stringify({
            chargectrl0: '00101100',
            stat0: '00000000',
            mask_id: '11000000',
          }),
        },
      ],
      pendingPayload: '',
    };

    renderWithProviders(<SerialLogPanel onChange={vi.fn()} value={chargerState} />);

    await user.click(screen.getByText('serialLog.actions.viewChargerStatus'));

    expect(screen.getByText('serialLog.chargerStatus.title')).toBeInTheDocument();
    expect(screen.getByText('CHARGECTRL0')).toBeInTheDocument();
  });

  it('opens ImportModeModal when "Import Mode" is clicked', async () => {
    const user = userEvent.setup();
    const modeState: SerialLogState = {
      entries: [
        {
          id: '1',
          timestamp: new Date().toISOString(),
          direction: 'inbound',
          payload: JSON.stringify({
            command: 'writeMode',
            index: 0,
            mode: {
              name: 'test mode',
              front: {
                pattern: {
                  type: 'simple',
                  name: 'test',
                  duration: 100,
                  changeAt: [{ ms: 0, output: 'high' }],
                },
              },
            },
          }),
        },
      ],
      pendingPayload: '',
    };

    renderWithProviders(<SerialLogPanel onChange={vi.fn()} value={modeState} />);

    await user.click(screen.getByText('serialLog.actions.importMode'));

    expect(screen.getByText('serialLog.importMode.title')).toBeInTheDocument();
    expect(screen.getByDisplayValue('test mode')).toBeInTheDocument();
  });
});
