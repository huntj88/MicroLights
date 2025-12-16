import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import toast from 'react-hot-toast';
import { describe, it, expect, vi, beforeEach, type Mock } from 'vitest';

import { SerialConnectButton } from './SerialConnectButton';
import { SerialFlashButton } from './SerialFlashButton';
import { SerialTestButton } from './SerialTestButton';
import type { Mode, ModePattern, HexColor } from '../../app/models/mode';
import { useSerialStore } from '../../app/providers/serial-store';

// Mock the serial store
vi.mock('../../app/providers/serial-store', () => ({
  useSerialStore: vi.fn(),
}));

// Mock react-hot-toast
vi.mock('react-hot-toast', () => ({
  default: {
    success: vi.fn(),
    error: vi.fn(),
  },
}));

describe('SerialButtons', () => {
  const mockSend = vi.fn();
  const mockConnect = vi.fn();
  const mockDisconnect = vi.fn();

  beforeEach(() => {
    vi.clearAllMocks();
    // Default to connected state
    (useSerialStore as unknown as Mock).mockImplementation(
      (selector?: (state: unknown) => unknown) => {
        const state = {
          status: 'connected',
          isConnected: true,
          isConnecting: false,
          isSupported: true,
          send: mockSend,
          connect: mockConnect,
          disconnect: mockDisconnect,
        };
        return selector ? selector(state) : state;
      },
    );
  });

  describe('SerialConnectButton', () => {
    it('renders not supported message when Web Serial is not supported', () => {
      (useSerialStore as unknown as Mock).mockImplementation(
        (selector?: (state: unknown) => unknown) => {
          const state = {
            isSupported: false,
            status: 'disconnected',
          };
          return selector ? selector(state) : state;
        },
      );

      render(<SerialConnectButton />);
      expect(screen.getByText('serialLog.notSupported')).toBeInTheDocument();
    });

    it('renders connect button when disconnected', () => {
      (useSerialStore as unknown as Mock).mockImplementation(
        (selector?: (state: unknown) => unknown) => {
          const state = {
            isSupported: true,
            status: 'disconnected',
            connect: mockConnect,
            disconnect: mockDisconnect,
          };
          return selector ? selector(state) : state;
        },
      );

      render(<SerialConnectButton />);
      const button = screen.getByRole('button', { name: /serialLog.actions.connect/i });
      expect(button).toBeInTheDocument();
      expect(button).toHaveTextContent('serialLog.actions.connect');
    });

    it('calls connect when clicked while disconnected', () => {
      (useSerialStore as unknown as Mock).mockImplementation(
        (selector?: (state: unknown) => unknown) => {
          const state = {
            isSupported: true,
            status: 'disconnected',
            connect: mockConnect,
            disconnect: mockDisconnect,
          };
          return selector ? selector(state) : state;
        },
      );

      render(<SerialConnectButton />);
      fireEvent.click(screen.getByRole('button', { name: /serialLog.actions.connect/i }));
      expect(mockConnect).toHaveBeenCalled();
    });

    it('renders disconnect button when connected', () => {
      // Default mock is connected
      render(<SerialConnectButton />);
      const button = screen.getByRole('button', { name: /serialLog.actions.disconnect/i });
      expect(button).toBeInTheDocument();
      expect(button).toHaveTextContent('serialLog.actions.disconnect');
    });

    it('calls disconnect when clicked while connected', () => {
      // Default mock is connected
      render(<SerialConnectButton />);
      fireEvent.click(screen.getByRole('button', { name: /serialLog.actions.disconnect/i }));
      expect(mockDisconnect).toHaveBeenCalled();
    });
  });

  describe('SerialTestButton', () => {
    const mockPattern: ModePattern = {
      type: 'simple',
      name: 'Test Pattern',
      duration: 1000,
      changeAt: [{ ms: 0, output: '#ff0000' as HexColor }],
    };

    it('renders correctly when connected', () => {
      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="front" disabled={false} />);
      expect(screen.getByRole('button', { name: /common.actions.test/i })).toBeInTheDocument();
    });

    it('does not render when not connected', () => {
      (useSerialStore as unknown as Mock).mockImplementation(
        (selector?: (state: unknown) => unknown) => {
          const state = {
            status: 'disconnected',
            isConnected: false,
            isConnecting: false,
            send: mockSend,
          };
          return selector ? selector(state) : state;
        },
      );

      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="front" disabled={false} />);
      expect(screen.queryByRole('button', { name: /common.actions.test/i })).not.toBeInTheDocument();
    });

    it('sends wrapped pattern for RGB type', async () => {
      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="front" disabled={false} />);
      fireEvent.click(screen.getByRole('button', { name: /common.actions.test/i }));

      await waitFor(() => {
        expect(mockSend).toHaveBeenCalledWith({
          name: 'transientTest',
          front: { pattern: mockPattern },
        });
        expect(toast.success).toHaveBeenCalledWith('common.actions.testSuccess');
      });
    });

    it('shows error toast on failure', async () => {
      mockSend.mockRejectedValueOnce(new Error('Failed'));
      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="front" disabled={false} />);
      fireEvent.click(screen.getByRole('button', { name: /common.actions.test/i }));

      await waitFor(() => {
        expect(toast.error).toHaveBeenCalledWith('common.actions.testError');
      });
    });

    it('sends wrapped pattern for Bulb type', async () => {
      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="case" disabled={false} />);
      fireEvent.click(screen.getByRole('button', { name: /common.actions.test/i }));

      await waitFor(() => {
        expect(mockSend).toHaveBeenCalledWith({
          name: 'transientTest',
          case: { pattern: mockPattern },
        });
      });
    });

    it('sends wrapped mode for Mode type', async () => {
      const mockMode: Mode = {
        name: 'Test Mode',
        front: { pattern: mockPattern },
      };
      render(<SerialTestButton data={mockMode} type="mode" disabled={false} />);
      fireEvent.click(screen.getByRole('button', { name: /common.actions.test/i }));

      await waitFor(() => {
        expect(mockSend).toHaveBeenCalledWith({
          ...mockMode,
          name: 'transientTest',
        });
      });
    });
  });

  describe('SerialFlashButton', () => {
    const mockMode: Mode = {
      name: 'Flash Mode',
      front: {
        pattern: {
          type: 'simple',
          name: 'P1',
          duration: 100,
          changeAt: [],
        },
      },
    };

    it('renders correctly when connected', () => {
      render(<SerialFlashButton mode={mockMode} disabled={false} />);
      expect(screen.getByRole('button', { name: /common.actions.flash/i })).toBeInTheDocument();
    });

    it('does not render when not connected', () => {
      (useSerialStore as unknown as Mock).mockImplementation(
        (selector?: (state: unknown) => unknown) => {
          const state = {
            status: 'disconnected',
            isConnected: false,
            isConnecting: false,
            send: mockSend,
          };
          return selector ? selector(state) : state;
        },
      );
      render(<SerialFlashButton mode={mockMode} disabled={false} />);
      expect(screen.queryByRole('button', { name: /common.actions.flash/i })).not.toBeInTheDocument();
    });

    it('sends flash command when index selected', async () => {
      render(<SerialFlashButton mode={mockMode} disabled={false} />);
      
      // Open modal
      fireEvent.click(screen.getByRole('button', { name: /common.actions.flash/i }));
      
      // Select index 1
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
        expect(toast.success).toHaveBeenCalledWith('common.actions.flashSuccess');
      });
    });

    it('shows error toast on failure', async () => {
      mockSend.mockRejectedValueOnce(new Error('Failed'));
      render(<SerialFlashButton mode={mockMode} disabled={false} />);
      
      // Open modal
      fireEvent.click(screen.getByRole('button', { name: /common.actions.flash/i }));
      
      // Select index 1
      fireEvent.click(screen.getByText('1'));

      // Click confirm
      const flashButtons = screen.getAllByText('common.actions.flash');
      fireEvent.click(flashButtons[flashButtons.length - 1]);

      await waitFor(() => {
        expect(toast.error).toHaveBeenCalledWith('common.actions.flashError');
      });
    });
  });
});
