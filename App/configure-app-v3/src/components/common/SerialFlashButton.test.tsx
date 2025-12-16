import { fireEvent, render, screen, waitFor } from '@testing-library/react';
import toast from 'react-hot-toast';
import { describe, expect, it, vi, beforeEach } from 'vitest';

import { SerialFlashButton } from './SerialFlashButton';
import { type Mode } from '../../app/models/mode';
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

describe('SerialFlashButton', () => {
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

    render(<SerialFlashButton mode={mockMode} disabled={false} />);
    expect(screen.queryByText('common.actions.flash')).not.toBeInTheDocument();
  });

  it('should render button when connected', () => {
    render(<SerialFlashButton mode={mockMode} disabled={false} />);
    expect(screen.getByText('common.actions.flash')).toBeInTheDocument();
  });

  it('should be disabled when disabled prop is true', () => {
    render(<SerialFlashButton mode={mockMode} disabled={true} />);
    expect(screen.getByText('common.actions.flash')).toBeDisabled();
  });

  it('should open modal on click', () => {
    render(<SerialFlashButton mode={mockMode} disabled={false} />);
    fireEvent.click(screen.getByText('common.actions.flash'));
    expect(screen.getByText('common.labels.index')).toBeInTheDocument();
  });

  it('should send flash command when index selected and confirmed', async () => {
    render(<SerialFlashButton mode={mockMode} disabled={false} />);

    // Open modal
    fireEvent.click(screen.getByText('common.actions.flash'));

    // Click index 1
    fireEvent.click(screen.getByText('1'));

    // Click confirm
    const flashButtons = screen.getAllByText('common.actions.flash');
    fireEvent.click(flashButtons[flashButtons.length - 1]);

    await waitFor(() => {
      expect(mockSend).toHaveBeenCalledWith({
        command: 'writeMode',
        index: 0,
        mode: mockMode,
      });
    });
  });

  it('should close modal on cancel', async () => {
    render(<SerialFlashButton mode={mockMode} disabled={false} />);

    // Open modal
    fireEvent.click(screen.getByText('common.actions.flash'));
    expect(screen.getByText('common.labels.index')).toBeInTheDocument();

    // Click Cancel
    fireEvent.click(screen.getByText('common.actions.cancel'));

    await waitFor(() => {
      expect(screen.queryByText('common.labels.index')).not.toBeInTheDocument();
    });
  });

  it('should show error toast on failure', async () => {
    mockSend.mockRejectedValueOnce(new Error('Failed'));
    render(<SerialFlashButton mode={mockMode} disabled={false} />);

    // Open modal
    fireEvent.click(screen.getByText('common.actions.flash'));

    // Click index 1
    fireEvent.click(screen.getByText('1'));

    // Click confirm
    const flashButtons = screen.getAllByText('common.actions.flash');
    fireEvent.click(flashButtons[flashButtons.length - 1]);

    await waitFor(() => {
      expect(toast.error).toHaveBeenCalledWith('common.actions.flashError');
    });
  });
});
