import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi, beforeEach, type Mock } from 'vitest';

import { SerialConnectButton } from './SerialConnectButton';
import { useSerialStore } from '../../app/providers/serial-store';

// Mock the serial store
vi.mock('../../app/providers/serial-store', () => ({
  useSerialStore: vi.fn(),
}));

describe('SerialConnectButton', () => {
  const mockConnect = vi.fn();
  const mockDisconnect = vi.fn();

  beforeEach(() => {
    vi.clearAllMocks();
  });

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
    (useSerialStore as unknown as Mock).mockImplementation(
      (selector?: (state: unknown) => unknown) => {
        const state = {
          isSupported: true,
          status: 'connected',
          connect: mockConnect,
          disconnect: mockDisconnect,
        };
        return selector ? selector(state) : state;
      },
    );

    render(<SerialConnectButton />);
    const button = screen.getByRole('button', { name: /serialLog.actions.disconnect/i });
    expect(button).toBeInTheDocument();
    expect(button).toHaveTextContent('serialLog.actions.disconnect');
  });

  it('calls disconnect when clicked while connected', () => {
    (useSerialStore as unknown as Mock).mockImplementation(
      (selector?: (state: unknown) => unknown) => {
        const state = {
          isSupported: true,
          status: 'connected',
          connect: mockConnect,
          disconnect: mockDisconnect,
        };
        return selector ? selector(state) : state;
      },
    );

    render(<SerialConnectButton />);
    fireEvent.click(screen.getByRole('button', { name: /serialLog.actions.disconnect/i }));
    expect(mockDisconnect).toHaveBeenCalled();
  });
});
