import userEvent from '@testing-library/user-event';
import { beforeEach, describe, expect, it, vi } from 'vitest';

import { type Mode } from '@/app/models/mode';
import { useModeStore } from '@/app/providers/mode-store';
import { usePatternStore } from '@/app/providers/pattern-store';
import { renderWithProviders, screen } from '@/test-utils/render-with-providers';

import { ImportModeModal } from './ImportModeModal';

describe('ImportModeModal', () => {
  const mockMode: Mode = {
    name: 'Test Mode',
    front: {
      pattern: {
        type: 'simple',
        name: 'Front Pattern',
        duration: 1000,
        changeAt: [{ ms: 0, output: 'high' }],
      },
    },
    case: {
      pattern: {
        type: 'simple',
        name: 'Case Pattern',
        duration: 1000,
        changeAt: [{ ms: 0, output: '#ff0000' }],
      },
    },
  };

  beforeEach(() => {
    useModeStore.setState({ modes: [] });
    usePatternStore.setState({ patterns: [] });
  });

  it('renders correctly', () => {
    renderWithProviders(
      <ImportModeModal isOpen={true} onClose={vi.fn()} mode={mockMode} />
    );

    expect(screen.getByText('serialLog.importMode.title')).toBeInTheDocument();
    expect(screen.getByDisplayValue('Test Mode')).toBeInTheDocument();
    expect(screen.getByDisplayValue('Front Pattern')).toBeInTheDocument();
    expect(screen.getByDisplayValue('Case Pattern')).toBeInTheDocument();
  });

  it('imports full mode and all patterns', async () => {
    const user = userEvent.setup();
    const onClose = vi.fn();
    
    renderWithProviders(
      <ImportModeModal isOpen={true} onClose={onClose} mode={mockMode} />
    );

    const importButton = screen.getByText('common.actions.import');
    await user.click(importButton);

    const savedMode = useModeStore.getState().getMode('Test Mode');
    expect(savedMode).toBeDefined();

    const savedFrontPattern = usePatternStore.getState().getPattern('Front Pattern');
    expect(savedFrontPattern).toBeDefined();

    const savedCasePattern = usePatternStore.getState().getPattern('Case Pattern');
    expect(savedCasePattern).toBeDefined();
  });

  it('renames patterns and updates references', async () => {
    const user = userEvent.setup();
    const onClose = vi.fn();
    
    renderWithProviders(
      <ImportModeModal isOpen={true} onClose={onClose} mode={mockMode} />
    );

    // Rename Front Pattern
    const frontInput = screen.getByDisplayValue('Front Pattern');
    await user.clear(frontInput);
    await user.type(frontInput, 'New Front Pattern');

    const importButton = screen.getByText('common.actions.import');
    await user.click(importButton);

    // Check if pattern saved with new name
    const savedPattern = usePatternStore.getState().getPattern('New Front Pattern');
    expect(savedPattern).toBeDefined();
    expect(usePatternStore.getState().getPattern('Front Pattern')).toBeUndefined();

    // Check if mode references new pattern name
    const savedMode = useModeStore.getState().getMode('Test Mode');
    expect(savedMode?.front?.pattern?.name).toBe('New Front Pattern');
  });

  it('shows overwrite warning for existing patterns', async () => {
    const onClose = vi.fn();

    // Pre-populate store with a conflicting pattern
    usePatternStore.getState().savePattern({
      type: 'simple',
      name: 'Front Pattern',
      duration: 500,
      changeAt: [{ ms: 0, output: 'low' }],
    });
    
    renderWithProviders(
      <ImportModeModal isOpen={true} onClose={onClose} mode={mockMode} />
    );

    // Should show warning
    expect(screen.getByText('serialLog.importMode.overwritePatternWarning')).toBeInTheDocument();

    // Button should say Overwrite
    expect(screen.getByText('common.actions.overwrite')).toBeInTheDocument();
  });

  it('shows overwrite warning for existing mode', async () => {
    const onClose = vi.fn();

    // Pre-populate store with a conflicting mode
    useModeStore.getState().saveMode({
      ...mockMode,
      name: 'Test Mode',
    });
    
    renderWithProviders(
      <ImportModeModal isOpen={true} onClose={onClose} mode={mockMode} />
    );

    // Should show warning
    expect(screen.getByText('serialLog.importMode.overwriteWarning')).toBeInTheDocument();

    // Button should say Overwrite
    expect(screen.getByText('common.actions.overwrite')).toBeInTheDocument();
  });
});
