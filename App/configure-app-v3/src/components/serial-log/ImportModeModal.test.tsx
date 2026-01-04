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
    expect(screen.getByText('Front Pattern')).toBeInTheDocument();
    expect(screen.getByText('Case Pattern')).toBeInTheDocument();
  });

  it('imports full mode and patterns by default', async () => {
    const user = userEvent.setup();
    const onClose = vi.fn();
    
    renderWithProviders(
      <ImportModeModal isOpen={true} onClose={onClose} mode={mockMode} />
    );

    const importButton = screen.getByText('common.actions.import');
    await user.click(importButton);

    const savedMode = useModeStore.getState().getMode('Test Mode');
    expect(savedMode).toBeDefined();

    // Default selection is empty for patterns
    const savedFrontPattern = usePatternStore.getState().getPattern('Front Pattern');
    expect(savedFrontPattern).toBeUndefined();
  });

  it('allows selecting patterns to import', async () => {
    const user = userEvent.setup();
    const onClose = vi.fn();
    
    renderWithProviders(
      <ImportModeModal isOpen={true} onClose={onClose} mode={mockMode} />
    );

    // Select Front Pattern
    const frontCheckbox = screen.getByLabelText(/Front Pattern/);
    await user.click(frontCheckbox);

    const importButton = screen.getByText('common.actions.import');
    await user.click(importButton);

    const savedFrontPattern = usePatternStore.getState().getPattern('Front Pattern');
    expect(savedFrontPattern).toBeDefined();
    
    const savedCasePattern = usePatternStore.getState().getPattern('Case Pattern');
    expect(savedCasePattern).toBeUndefined();
  });

  it('allows importing only patterns without mode', async () => {
    const user = userEvent.setup();
    const onClose = vi.fn();
    
    renderWithProviders(
      <ImportModeModal isOpen={true} onClose={onClose} mode={mockMode} />
    );

    // Deselect Import Mode
    const modeCheckbox = screen.getByLabelText('serialLog.importMode.importFullMode');
    await user.click(modeCheckbox);

    // Select Case Pattern
    const caseCheckbox = screen.getByLabelText(/Case Pattern/);
    await user.click(caseCheckbox);

    const importButton = screen.getByText('common.actions.import');
    await user.click(importButton);

    const savedMode = useModeStore.getState().getMode('Test Mode');
    expect(savedMode).toBeUndefined();

    const savedCasePattern = usePatternStore.getState().getPattern('Case Pattern');
    expect(savedCasePattern).toBeDefined();
  });

  it('shows overwrite warning for existing patterns', async () => {
    const user = userEvent.setup();
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

    // Select Front Pattern (which exists)
    const frontCheckbox = screen.getByLabelText(/Front Pattern/);
    await user.click(frontCheckbox);

    // Should show warning
    expect(screen.getByText('serialLog.importMode.overwritePatternWarning')).toBeInTheDocument();

    // Button should say Overwrite
    expect(screen.getByText('common.actions.overwrite')).toBeInTheDocument();
  });
});
