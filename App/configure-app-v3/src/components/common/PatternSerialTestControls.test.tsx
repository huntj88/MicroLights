import { fireEvent, render, screen, waitFor } from '@testing-library/react';
import toast from 'react-hot-toast';
import { beforeEach, describe, expect, it, vi } from 'vitest';

import { PatternSerialTestControls } from './PatternSerialTestControls';
import { type HexColor, type ModePattern } from '../../app/models/mode';
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

const createMockPattern = (): ModePattern => ({
  type: 'simple',
  name: 'Test Pattern',
  duration: 1000,
  changeAt: [{ ms: 0, output: '#ff0000' as HexColor }],
});

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

describe('PatternSerialTestControls', () => {
  it('should render nothing when not connected', () => {
    (useSerialStore as unknown as ReturnType<typeof vi.fn>).mockImplementation(
      <T,>(selector: (state: SerialStoreState) => T) => {
        const state = { status: 'disconnected', send: mockSend } as unknown as SerialStoreState;
        return selector(state);
      },
    );

    render(<PatternSerialTestControls data={createMockPattern()} disabled={false} />);
    expect(screen.queryByText('common.actions.testFront')).not.toBeInTheDocument();
  });

  it('should send preview to front', async () => {
    const mockPattern = createMockPattern();

    render(<PatternSerialTestControls data={mockPattern} disabled={false} />);
    fireEvent.click(screen.getByText('common.actions.testFront'));

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

  it('should send preview to case', async () => {
    const mockPattern = createMockPattern();

    render(<PatternSerialTestControls data={mockPattern} disabled={false} />);
    fireEvent.click(screen.getByText('common.actions.testCase'));

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

  it('should render front before case', () => {
    render(<PatternSerialTestControls data={createMockPattern()} disabled={false} />);

    expect(screen.getAllByRole('button').map(button => button.textContent)).toEqual([
      'common.actions.testFront',
      'common.actions.testCase',
    ]);
  });

  it('should send preview to both targets when both buttons are active', async () => {
    const mockPattern = createMockPattern();

    render(<PatternSerialTestControls data={mockPattern} disabled={false} />);

    fireEvent.click(screen.getByText('common.actions.testFront'));

    await waitFor(() => {
      expect(mockSend).toHaveBeenNthCalledWith(1, {
        command: 'writeMode',
        index: 0,
        mode: {
          name: 'transientTest',
          front: { pattern: mockPattern },
        },
      });
    });

    fireEvent.click(screen.getByText('common.actions.testCase'));

    await waitFor(() => {
      expect(mockSend).toHaveBeenNthCalledWith(2, {
        command: 'writeMode',
        index: 0,
        mode: {
          name: 'transientTest',
          front: { pattern: mockPattern },
          case: { pattern: mockPattern },
        },
      });
    });
  });
});