import { render, screen, fireEvent } from '@testing-library/react';
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

// Mock react-i18next
vi.mock('react-i18next', () => ({
  useTranslation: () => ({
    t: (key: string) => {
      const translations: Record<string, string> = {
        'common.actions.test': 'Test on Device',
        'common.actions.flash': 'Flash to Device',
        'serialLog.notSupported': 'Web Serial not supported',
        'serialLog.actions.connect': 'Connect',
        'serialLog.actions.disconnect': 'Disconnect',
      };
      return translations[key] || key;
    },
  }),
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
      expect(screen.getByText('Web Serial not supported')).toBeInTheDocument();
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
      const button = screen.getByRole('button', { name: /connect/i });
      expect(button).toBeInTheDocument();
      expect(button).toHaveTextContent('Connect');
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
      fireEvent.click(screen.getByRole('button', { name: /connect/i }));
      expect(mockConnect).toHaveBeenCalled();
    });

    it('renders disconnect button when connected', () => {
      // Default mock is connected
      render(<SerialConnectButton />);
      const button = screen.getByRole('button', { name: /disconnect/i });
      expect(button).toBeInTheDocument();
      expect(button).toHaveTextContent('Disconnect');
    });

    it('calls disconnect when clicked while connected', () => {
      // Default mock is connected
      render(<SerialConnectButton />);
      fireEvent.click(screen.getByRole('button', { name: /disconnect/i }));
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
      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="front" />);
      expect(screen.getByRole('button', { name: /test on device/i })).toBeInTheDocument();
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

      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="front" />);
      expect(screen.queryByRole('button', { name: /test on device/i })).not.toBeInTheDocument();
    });

    it('sends wrapped pattern for RGB type', () => {
      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="front" />);
      fireEvent.click(screen.getByRole('button', { name: /test on device/i }));

      expect(mockSend).toHaveBeenCalledWith({
        name: 'transientTest',
        front: { pattern: mockPattern },
      });
    });

    it('sends wrapped pattern for Bulb type', () => {
      render(<SerialTestButton data={mockPattern} type="pattern" patternTarget="case" />);
      fireEvent.click(screen.getByRole('button', { name: /test on device/i }));

      expect(mockSend).toHaveBeenCalledWith({
        name: 'transientTest',
        case: { pattern: mockPattern },
      });
    });

    it('sends wrapped mode for Mode type', () => {
      const mockMode: Mode = {
        name: 'Test Mode',
        front: { pattern: mockPattern },
      };
      render(<SerialTestButton data={mockMode} type="mode" />);
      fireEvent.click(screen.getByRole('button', { name: /test on device/i }));

      expect(mockSend).toHaveBeenCalledWith({
        ...mockMode,
        name: 'transientTest',
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
      render(<SerialFlashButton mode={mockMode} />);
      expect(screen.getByRole('button', { name: /flash to device/i })).toBeInTheDocument();
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
      render(<SerialFlashButton mode={mockMode} />);
      expect(screen.queryByRole('button', { name: /flash to device/i })).not.toBeInTheDocument();
    });

    it('sends raw mode data', () => {
      render(<SerialFlashButton mode={mockMode} />);
      fireEvent.click(screen.getByRole('button', { name: /flash to device/i }));

      expect(mockSend).toHaveBeenCalledWith(mockMode);
    });
  });
});
