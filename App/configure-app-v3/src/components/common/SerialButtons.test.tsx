import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi, beforeEach, type Mock } from 'vitest';

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
    (useSerialStore as unknown as Mock).mockImplementation((selector?: (state: unknown) => unknown) => {
      const state = {
        status: 'connected',
        isConnected: true,
        isConnecting: false,
        send: mockSend,
        connect: mockConnect,
        disconnect: mockDisconnect,
      };
      return selector ? selector(state) : state;
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
      (useSerialStore as unknown as Mock).mockImplementation((selector?: (state: unknown) => unknown) => {
        const state = {
          status: 'disconnected',
          isConnected: false,
          isConnecting: false,
          send: mockSend,
        };
        return selector ? selector(state) : state;
      });
      
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
      (useSerialStore as unknown as Mock).mockImplementation((selector?: (state: unknown) => unknown) => {
        const state = {
          status: 'disconnected',
          isConnected: false,
          isConnecting: false,
          send: mockSend,
        };
        return selector ? selector(state) : state;
      });
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
