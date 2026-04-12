import { fireEvent, render, screen, waitFor } from '@testing-library/react';
import toast from 'react-hot-toast';
import { beforeEach, describe, expect, it, vi } from 'vitest';

import { ModeSerialTestButton } from './ModeSerialTestButton';
import { type Mode } from '../../app/models/mode';
import { useSerialStore, type SerialStoreState } from '../../app/providers/serial-store';

vi.mock('../../app/providers/serial-store', () => ({
  useSerialStore: vi.fn(),
}));

vi.mock('react-hot-toast', () => ({
  default: {
    success: vi.fn(),
    error: vi.fn(),
  },
}));

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

describe('ModeSerialTestButton', () => {
  it('should render nothing when not connected', () => {
    (useSerialStore as unknown as ReturnType<typeof vi.fn>).mockImplementation(
      <T,>(selector: (state: SerialStoreState) => T) => {
        const state = { status: 'disconnected', send: mockSend } as unknown as SerialStoreState;
        return selector(state);
      },
    );

    render(<ModeSerialTestButton data={mockMode} disabled={false} />);
    expect(screen.queryByText('common.actions.test')).not.toBeInTheDocument();
  });

  it('should render button when connected', () => {
    render(<ModeSerialTestButton data={mockMode} disabled={false} />);
    expect(screen.getByText('common.actions.test')).toBeInTheDocument();
  });

  it('should be disabled when disabled prop is true', () => {
    render(<ModeSerialTestButton data={mockMode} disabled={true} />);
    expect(screen.getByText('common.actions.test')).toBeDisabled();
  });

  it('should send test command on click', async () => {
    render(<ModeSerialTestButton data={mockMode} disabled={false} />);
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
    render(<ModeSerialTestButton data={mockMode} disabled={true} />);
    fireEvent.click(screen.getByText('common.actions.test'));
    expect(mockSend).not.toHaveBeenCalled();
  });

  it('should show error toast on failure', async () => {
    mockSend.mockRejectedValueOnce(new Error('Failed'));
    render(<ModeSerialTestButton data={mockMode} disabled={false} />);
    fireEvent.click(screen.getByText('common.actions.test'));

    await waitFor(() => {
      expect(toast.error).toHaveBeenCalledWith('common.actions.testError');
    });
  });

  it('should toggle auto-sync and send updates', async () => {
    const { rerender } = render(<ModeSerialTestButton data={mockMode} disabled={false} />);

    fireEvent.click(screen.getByText('common.actions.test'));

    await waitFor(() => {
      expect(mockSend).toHaveBeenCalledTimes(1);
    });

    expect(screen.getByText('common.actions.stopTest')).toBeInTheDocument();

    const updatedMode = { ...mockMode, name: 'Updated' };
    rerender(<ModeSerialTestButton data={updatedMode} disabled={false} />);

    await waitFor(
      () => {
        expect(mockSend).toHaveBeenCalledTimes(2);
      },
      { timeout: 1000 },
    );

    fireEvent.click(screen.getByText('common.actions.stopTest'));

    const anotherMode = { ...mockMode, name: 'Another' };
    rerender(<ModeSerialTestButton data={anotherMode} disabled={false} />);

    await new Promise(r => setTimeout(r, 200));
    expect(mockSend).toHaveBeenCalledTimes(2);
  });

  it('should keep auto-sync enabled through validation errors and resume when valid again', async () => {
    const { rerender } = render(<ModeSerialTestButton data={mockMode} disabled={false} />);

    fireEvent.click(screen.getByText('common.actions.test'));

    await waitFor(() => {
      expect(mockSend).toHaveBeenCalledTimes(1);
    });

    const invalidMode = { ...mockMode, name: 'Invalid' };
    rerender(<ModeSerialTestButton data={invalidMode} disabled={true} />);

    expect(screen.getByText('common.actions.stopTest')).toBeEnabled();

    await new Promise(r => setTimeout(r, 200));
    expect(mockSend).toHaveBeenCalledTimes(1);

    rerender(<ModeSerialTestButton data={invalidMode} disabled={false} />);

    await waitFor(() => {
      expect(mockSend).toHaveBeenCalledTimes(2);
    });
  });
});