import { fireEvent, render, screen, waitFor, act } from '@testing-library/react';
import { describe, expect, it, vi, beforeEach, type Mock } from 'vitest';

import { SerialLogPage } from './SerialLogPage';
import { serialManager } from '../../app/providers/serial-manager';
import { useSerialStore, type SerialStoreState } from '../../app/providers/serial-store';

// Mock the store
vi.mock('../../app/providers/serial-store', () => ({
  useSerialStore: vi.fn(),
}));

// Mock serial manager
vi.mock('../../app/providers/serial-manager', () => ({
  serialManager: {
    send: vi.fn(),
    on: vi.fn(() => () => undefined),
  },
}));

// Mock components that might cause issues or aren't needed for this test
vi.mock('@/components/common/SerialConnectButton', () => ({
  SerialConnectButton: () => <button>Connect</button>,
}));

vi.mock('@/components/serial-log/SerialLogPanel', () => ({
  SerialLogPanel: () => <div>Log Panel</div>,
}));

describe('SerialLogPage', () => {
  const mockSend = vi.fn();

  beforeEach(() => {
    vi.clearAllMocks();
    (useSerialStore as unknown as ReturnType<typeof vi.fn>).mockImplementation(
      <T,>(selector: (state: SerialStoreState) => T) => {
        const state = {
          status: 'connected',
          send: mockSend,
          logs: [],
          autoscroll: true,
          clearLogs: vi.fn(),
          setAutoscroll: vi.fn(),
        } as unknown as SerialStoreState;
        return selector(state);
      },
    );
  });

  it('renders debug buttons', () => {
    render(<SerialLogPage />);
    expect(screen.getByText('Read Modes')).toBeInTheDocument();
    expect(screen.getByText('Settings')).toBeInTheDocument();
    expect(screen.getByText('DFU')).toBeInTheDocument();
  });

  it('sends readMode command for all slots', async () => {
    render(<SerialLogPage />);
    fireEvent.click(screen.getByText('Read Modes'));

    await waitFor(() => {
      expect(mockSend).toHaveBeenCalledTimes(6);
    });

    for (let i = 0; i < 6; i++) {
      expect(mockSend).toHaveBeenCalledWith({ command: 'readMode', index: i });
    }
  });

  it('opens settings modal and sends writeSettings command on save', async () => {
    let dataListener: ((line: string, json: unknown) => void) | undefined;

    // Update mock to capture listener
    (serialManager.on as Mock).mockImplementation((event: string, listener: (line: string, json: unknown) => void) => {
      if (event === 'data') {
        dataListener = listener;
      }
      return () => undefined;
    });

    render(<SerialLogPage />);
    fireEvent.click(screen.getByText('Settings'));

    // Modal should be open
    expect(screen.getByText('Configure Settings')).toBeInTheDocument();

    // It should have sent readSettings
    // eslint-disable-next-line @typescript-eslint/unbound-method
    expect(serialManager.send as Mock).toHaveBeenCalledWith({ command: 'readSettings' });

    // Simulate response
    expect(dataListener).toBeDefined();
    act(() => {
      if (dataListener) {
        dataListener('', {
          modeCount: 5,
          minutesUntilAutoOff: 30,
          minutesUntilLockAfterAutoOff: 60,
        });
      }
    });

    // Wait for save button to be enabled (loading finished)
    const saveButton = screen.getByText('common.actions.save').closest('button');
    await waitFor(() => {
      expect(saveButton).not.toBeDisabled();
    });

    // Simulate user input (optional, or just click save)
    fireEvent.click(screen.getByText('common.actions.save'));

    // Should send writeSettings
    await waitFor(() => {
      // eslint-disable-next-line @typescript-eslint/unbound-method
      expect(serialManager.send as Mock).toHaveBeenCalledWith({
        command: 'writeSettings',
        modeCount: 5,
        minutesUntilAutoOff: 30,
        minutesUntilLockAfterAutoOff: 60,
      });
    });
  });

  it('sends dfu command', () => {
    render(<SerialLogPage />);
    fireEvent.click(screen.getByText('DFU'));
    expect(mockSend).toHaveBeenCalledWith({ command: 'dfu' });
  });

  it('disables buttons when not connected', () => {
    (useSerialStore as unknown as ReturnType<typeof vi.fn>).mockImplementation(
      <T,>(selector: (state: SerialStoreState) => T) => {
        const state = {
          status: 'disconnected',
          send: mockSend,
          logs: [],
          autoscroll: true,
          clearLogs: vi.fn(),
          setAutoscroll: vi.fn(),
        } as unknown as SerialStoreState;
        return selector(state);
      },
    );

    render(<SerialLogPage />);
    expect(screen.getByText('Read Modes').closest('button')).toBeDisabled();
    expect(screen.getByText('Settings').closest('button')).toBeDisabled();
    expect(screen.getByText('DFU').closest('button')).toBeDisabled();
  });
});
