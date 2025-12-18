import { fireEvent, render, screen, waitFor } from '@testing-library/react';
import toast from 'react-hot-toast';
import { describe, expect, it, vi, beforeEach } from 'vitest';

import { SerialTestButton } from './SerialTestButton';
import { type Mode, type ModePattern, type HexColor } from '../../app/models/mode';
import { useSerialStore, type SerialStoreState } from '../../app/providers/serial-store';

// Mock the store
vi.mock('../../app/providers/serial-store', () => ({
  useSerialStore: vi.fn(),
}));

// Mock toast
vi.mock('react-hot-toast', () => ({
  default: {
    success: vi.fn(),
    error: vi.fn(),
  },
}));

describe('SerialTestButton', () => {
  const mockMode: Mode = {
    name: 'Test Mode',
    front: undefined,
    case: undefined,
  };

  const mockSend = vi.fn();

  beforeEach(() => {
    vi.clearAllMocks();
    (useSerialStore as unknown as ReturnType<typeof vi.fn>).mockImplementation(
      <T,>(selector: (state: SerialStoreState) => T) => {
        const state = {
          status: 'connected',
          send: mockSend,
        } as unknown as SerialStoreState;
        return selector(state);
      },
    );
  });

  it('should render nothing when not connected', () => {
    (useSerialStore as unknown as ReturnType<typeof vi.fn>).mockImplementation(
      <T,>(selector: (state: SerialStoreState) => T) => {
        const state = { status: 'disconnected', send: mockSend } as unknown as SerialStoreState;
        return selector(state);
      },
    );

    render(<SerialTestButton data={mockMode} type="mode" disabled={false} />);
    expect(screen.queryByText('common.actions.test')).not.toBeInTheDocument();
  });

  it('should render button when connected', () => {
    render(<SerialTestButton data={mockMode} type="mode" disabled={false} />);
    expect(screen.getByText('common.actions.test')).toBeInTheDocument();
  });

  it('should be disabled when disabled prop is true', () => {
    render(<SerialTestButton data={mockMode} type="mode" disabled={true} />);
    expect(screen.getByText('common.actions.test')).toBeDisabled();
  });

  it('should send test command on click', async () => {
    render(<SerialTestButton data={mockMode} type="mode" disabled={false} />);
    fireEvent.click(screen.getByText('common.actions.test'));

    await waitFor(() => {
      expect(mockSend).toHaveBeenCalledWith({
        command: 'writeMode',
        index: 0,
        mode: {
          ...mockMode,
          name: 'transientTest',
        },
      });
    });
  });

  it('should not send command if disabled', () => {
    render(<SerialTestButton data={mockMode} type="mode" disabled={true} />);
    fireEvent.click(screen.getByText('common.actions.test'));
    expect(mockSend).not.toHaveBeenCalled();
  });

  it('should send wrapped pattern for RGB type', async () => {
    const mockPattern: ModePattern = {
      type: 'simple',
      name: 'Test Pattern',
      duration: 1000,
      changeAt: [{ ms: 0, output: '#ff0000' as HexColor }],
    };

    render(
      <SerialTestButton data={mockPattern} type="pattern" patternTarget="front" disabled={false} />,
    );
    fireEvent.click(screen.getByText('common.actions.test'));

    await waitFor(() => {
      expect(mockSend).toHaveBeenCalledWith({
        command: 'writeMode',
        index: 0,
        mode: {
          name: 'transientTest',
          front: { pattern: mockPattern },
        },
      });
      expect(toast.success).toHaveBeenCalledWith('common.actions.testSuccess');
    });
  });

  it('should send wrapped pattern for Bulb type', async () => {
    const mockPattern: ModePattern = {
      type: 'simple',
      name: 'Test Pattern',
      duration: 1000,
      changeAt: [{ ms: 0, output: '#ff0000' as HexColor }],
    };

    render(
      <SerialTestButton data={mockPattern} type="pattern" patternTarget="case" disabled={false} />,
    );
    fireEvent.click(screen.getByText('common.actions.test'));

    await waitFor(() => {
      expect(mockSend).toHaveBeenCalledWith({
        command: 'writeMode',
        index: 0,
        mode: {
          name: 'transientTest',
          case: { pattern: mockPattern },
        },
      });
    });
  });

  it('should show error toast on failure', async () => {
    mockSend.mockRejectedValueOnce(new Error('Failed'));
    render(<SerialTestButton data={mockMode} type="mode" disabled={false} />);
    fireEvent.click(screen.getByText('common.actions.test'));

    await waitFor(() => {
      expect(toast.error).toHaveBeenCalledWith('common.actions.testError');
    });
  });
});
