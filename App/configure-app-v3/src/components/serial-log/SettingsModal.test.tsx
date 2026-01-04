import { fireEvent, render, screen, waitFor } from '@testing-library/react';
import { act } from 'react';
import { beforeEach, describe, expect, it, vi } from 'vitest';

import { serialManager } from '@/app/providers/serial-manager';

import { SettingsModal } from './SettingsModal';

// Mock dependencies
vi.mock('react-i18next', () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock('react-hot-toast', () => ({
  default: {
    success: vi.fn(),
    error: vi.fn(),
  },
}));

vi.mock('@/app/providers/serial-manager', () => ({
  serialManager: {
    send: vi.fn(),
    on: vi.fn(),
  },
}));

describe('SettingsModal', () => {
  const mockOnClose = vi.fn();
  let dataCallback: (line: string, json: unknown) => void;

  beforeEach(() => {
    vi.clearAllMocks();
    // Capture the data listener
    // eslint-disable-next-line @typescript-eslint/unbound-method
    vi.mocked(serialManager.on).mockImplementation((event, callback) => {
      if (event === 'data') {
        dataCallback = callback as (line: string, json: unknown) => void;
      }
      return vi.fn();
    });
  });

  it('renders correctly with dynamic settings', async () => {
    render(<SettingsModal isOpen={true} onClose={mockOnClose} />);

    // Verify read command was sent
    // eslint-disable-next-line @typescript-eslint/unbound-method
    expect(serialManager.send).toHaveBeenCalledWith({ command: 'readSettings' });

    // Simulate receiving data
    const testData = {
      settings: {
        command: 'writeSettings',
        modeCount: 3,
        equationEvalIntervalMs: 0,
      },
      defaults: {
        modeCount: 0,
        minutesUntilAutoOff: 90,
        minutesUntilLockAfterAutoOff: 10,
        equationEvalIntervalMs: 20,
        enableChargerSerial: false,
      },
    };

    act(() => {
      dataCallback('', testData);
    });

    // Wait for loading to finish and form to appear
    await waitFor(() => {
      expect(screen.getByText('Mode Count')).toBeInTheDocument();
    });

    // Check Mode Count (value 3 from settings)
    const modeCountInput = screen.getByLabelText('Mode Count');
    expect(modeCountInput.value).toBe('3');
    expect(screen.getByText('Default: 0')).toBeInTheDocument();

    // Check Equation Eval Interval (value 0 from settings)
    const eqInput = screen.getByLabelText('Equation Eval Interval Ms');
    expect(eqInput.value).toBe('0');
    expect(screen.getByText('Default: 20')).toBeInTheDocument();

    // Check Minutes Until Auto Off (value 90 from defaults, as it's missing in settings)
    const autoOffInput = screen.getByLabelText('Minutes Until Auto Off');
    expect(autoOffInput.value).toBe('90');
    expect(screen.getByText('Default: 90')).toBeInTheDocument();

    // Check Enable Charger Serial (boolean)
    // The label might be "Enable Charger Serial"
    const chargerCheckbox = screen.getByLabelText('Enable Charger Serial');
    expect(chargerCheckbox).not.toBeChecked(); // default is false, not in settings
    expect(screen.getByText('Default: false')).toBeInTheDocument();
  });

  it('updates values and sends only changes on save', async () => {
    render(<SettingsModal isOpen={true} onClose={mockOnClose} />);

    const testData = {
      settings: {
        command: 'writeSettings',
        modeCount: 3,
      },
      defaults: {
        modeCount: 0,
        minutesUntilAutoOff: 90,
      },
    };

    act(() => {
      dataCallback('', testData);
    });

    await waitFor(() => {
      expect(screen.getByText('Mode Count')).toBeInTheDocument();
    });

    // Change Mode Count to 5
    const modeCountInput = screen.getByLabelText('Mode Count');
    fireEvent.change(modeCountInput, { target: { value: '5' } });

    // Change Auto Off to 90 (same as default) - should not be sent
    // Actually it is already 90. Let's change it to 100 then back to 90?
    // Or just leave it.
    // Let's change Auto Off to 60 (diff from default)
    const autoOffInput = screen.getByLabelText('Minutes Until Auto Off');
    fireEvent.change(autoOffInput, { target: { value: '60' } });

    // Save
    const saveButton = screen.getByText('common.actions.save');
    fireEvent.click(saveButton);

    await waitFor(() => {
      // eslint-disable-next-line @typescript-eslint/unbound-method
      expect(serialManager.send).toHaveBeenCalledWith({
        command: 'writeSettings',
        modeCount: 5,
        minutesUntilAutoOff: 60,
      });
    });
  });
});
