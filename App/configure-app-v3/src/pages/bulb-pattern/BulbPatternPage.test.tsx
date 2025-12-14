import userEvent from '@testing-library/user-event';
import { describe, expect, it, vi } from 'vitest';

import { hexColorSchema, type SimplePattern } from '@/app/models/mode';
import { usePatternStore } from '@/app/providers/pattern-store';
import { renderWithProviders, screen, waitFor, within } from '@/test-utils/render-with-providers';

import { BulbPatternPage } from './BulbPatternPage';

describe('BulbPatternPage', () => {
  const setup = () => {
    usePatternStore.setState({ patterns: [] });
    localStorage.removeItem('flow-art-forge-patterns');
    return {
      user: userEvent.setup(),
    };
  };

  it('renders the bulb pattern editor', () => {
    setup();
    renderWithProviders(<BulbPatternPage />);

    expect(
      screen.getByRole('heading', { level: 2, name: /bulb pattern studio/i }),
    ).toBeInTheDocument();
  });

  it('filters out color patterns from the saved patterns list', () => {
    setup();
    const binaryPattern: SimplePattern = {
      type: 'simple',
      name: 'Binary Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: 'high' }],
    };
    const colorPattern: SimplePattern = {
      type: 'simple',
      name: 'Color Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
    };

    usePatternStore.getState().savePattern(binaryPattern);
    usePatternStore.getState().savePattern(colorPattern);

    renderWithProviders(<BulbPatternPage />);

    const chooser = screen.getByLabelText(/saved patterns/i);

    // Check that Binary Pattern is an option
    expect(within(chooser).getByRole('option', { name: 'Binary Pattern' })).toBeInTheDocument();

    // Check that Color Pattern is NOT an option
    expect(
      within(chooser).queryByRole('option', { name: 'Color Pattern' }),
    ).not.toBeInTheDocument();
  });

  it('disables save button on initial load due to invalid default pattern', () => {
    setup();
    renderWithProviders(<BulbPatternPage />);

    const saveButton = screen.getByRole('button', { name: /save pattern/i });
    expect(saveButton).toBeDisabled();
  });

  it('disables save button when switching to new pattern', async () => {
    const { user } = setup();
    const storedPattern: SimplePattern = {
      type: 'simple',
      name: 'Valid Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: 'high' }],
    };
    usePatternStore.getState().savePattern(storedPattern);

    renderWithProviders(<BulbPatternPage />);

    // Select valid pattern
    const chooser = screen.getByLabelText(/saved patterns/i);
    await user.selectOptions(chooser, storedPattern.name);

    // Switch to New Pattern
    await user.selectOptions(chooser, '');

    const saveButton = screen.getByRole('button', { name: /save pattern/i });
    expect(saveButton).toBeDisabled();
  });

  it('prompts before overwriting an existing pattern when saving', async () => {
    const { user } = setup();
    renderWithProviders(<BulbPatternPage />);

    const saveButton = screen.getByRole('button', { name: /save pattern/i });
    const addButton = screen.getByRole('button', { name: /add step/i });
    const nameInput = screen.getByRole('textbox', { name: /pattern name/i });

    await user.type(nameInput, 'My Pattern');
    await user.click(addButton);

    let dialog = await screen.findByRole('dialog');
    await user.click(within(dialog).getByRole('button', { name: /add step/i }));
    expect(saveButton).toBeEnabled();

    await user.click(saveButton);

    const chooser = screen.getByLabelText(/saved patterns/i);
    const options = within(chooser).getAllByRole('option');
    expect(options).toHaveLength(2);
    expect(chooser).toHaveValue('My Pattern');

    // Make a change to trigger the overwrite prompt (otherwise save is disabled)
    await user.click(addButton);

    dialog = await screen.findByRole('dialog');
    await user.click(within(dialog).getByRole('button', { name: /add step/i }));

    // We need to select "New Pattern" or change the name to trigger overwrite logic for a *different* pattern name
    // But here we are saving "My Pattern" again.
    // The logic in BulbPatternPage is:
    // const existing = patterns.find(p => p.name === patternState.name);
    // if (existing && existing.name !== selectedPatternName) { ... confirm ... }

    // So if we are editing "My Pattern" (selectedPatternName="My Pattern") and saving as "My Pattern",
    // it just saves without confirm.

    // To trigger confirm, we must be in "New Pattern" mode (selectedPatternName="")
    // OR have a different pattern selected, but try to save with a name that already exists.

    // Let's switch to "New Pattern" mode but keep the name "My Pattern"
    await user.selectOptions(chooser, '');

    // When we switch to "New Pattern", the state is reset to empty.
    // We need to re-enter the name "My Pattern" and add a step to make it valid and conflicting.
    await user.type(nameInput, 'My Pattern');
    await user.click(addButton);
    dialog = await screen.findByRole('dialog');
    await user.click(within(dialog).getByRole('button', { name: /add step/i }));

    const confirmSpy = vi.spyOn(window, 'confirm').mockImplementation(() => true);

    await user.click(saveButton);

    expect(confirmSpy).toHaveBeenCalledWith(expect.stringMatching(/replace it/i));
    confirmSpy.mockRestore();
  });

  it('shows validation errors when saving an invalid pattern', async () => {
    const { user } = setup();
    renderWithProviders(<BulbPatternPage />);

    const saveButton = screen.getByRole('button', { name: /save pattern/i });
    await user.click(saveButton);

    expect(screen.getByText(/at least one change event is required/i)).toBeInTheDocument();
  });

  it('disables save button when loaded pattern is unchanged', async () => {
    const { user } = setup();
    const storedPattern: SimplePattern = {
      type: 'simple',
      name: 'My Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: 'high' }],
    };
    usePatternStore.getState().savePattern(storedPattern);

    renderWithProviders(<BulbPatternPage />);

    const chooser = screen.getByLabelText(/saved patterns/i);
    await user.selectOptions(chooser, storedPattern.name);

    const saveButton = screen.getByRole('button', { name: /save pattern/i });
    await waitFor(() => {
      expect(saveButton).toBeDisabled();
    });

    // Change name
    const nameInput = screen.getByRole('textbox', { name: /pattern name/i });
    await user.clear(nameInput);
    await user.type(nameInput, 'My Pattern Modified');

    expect(saveButton).toBeEnabled();
  });

  it('deletes the selected pattern after confirmation', async () => {
    const { user } = setup();
    const storedPattern: SimplePattern = {
      type: 'simple',
      name: 'Stored Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: 'high' }],
    };
    usePatternStore.getState().savePattern(storedPattern);

    renderWithProviders(<BulbPatternPage />);

    const chooser = screen.getByLabelText(/saved patterns/i);
    await user.selectOptions(chooser, storedPattern.name);
    expect(screen.getByRole('textbox', { name: /pattern name/i })).toHaveValue(storedPattern.name);

    const confirmSpy = vi.spyOn(window, 'confirm').mockImplementation(() => true);

    await user.click(screen.getByRole('button', { name: /delete pattern/i }));

    expect(confirmSpy).toHaveBeenCalledWith(expect.stringMatching(/delete the pattern/i));
    confirmSpy.mockRestore();

    expect(usePatternStore.getState().patterns).toHaveLength(0);
    expect(chooser).toHaveValue('');
  });

  it('resets the builder when switching back to the new pattern option', async () => {
    const { user } = setup();
    renderWithProviders(<BulbPatternPage />);

    const addButton = screen.getByRole('button', { name: /add step/i });
    await user.click(addButton);

    const dialog = await screen.findByRole('dialog');
    await user.click(within(dialog).getByRole('button', { name: /add step/i }));

    expect(screen.getByRole('heading', { name: /pattern steps/i })).toBeInTheDocument();

    const chooser = screen.getByLabelText(/saved patterns/i);
    await user.selectOptions(chooser, '');

    expect(chooser).toHaveValue('');
    expect(screen.getAllByText(/no steps have been added yet/i)).toHaveLength(2);
    expect(screen.getByRole('textbox', { name: /pattern name/i })).toHaveValue('');
  });
});
