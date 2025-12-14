import userEvent from '@testing-library/user-event';
import { describe, expect, it, vi } from 'vitest';

import { hexColorSchema, type SimplePattern } from '@/app/models/mode';
import { usePatternStore } from '@/app/providers/pattern-store';
import { renderWithProviders, screen, waitFor, within } from '@/test-utils/render-with-providers';

import { RgbPatternPage } from './RgbPatternPage';

describe('RgbPatternPage', () => {
  const setup = () => {
    usePatternStore.setState({ patterns: [] });
    localStorage.removeItem('flow-art-forge-patterns');
    return {
      user: userEvent.setup(),
    };
  };

  it('shows the simple method by default and hides the equation method', () => {
    setup();
    renderWithProviders(<RgbPatternPage />);

    expect(screen.getByRole('heading', { level: 3, name: /simple method/i })).toBeInTheDocument();
    expect(
      screen.queryByRole('heading', { level: 3, name: /equation method/i }),
    ).not.toBeInTheDocument();
  });

  it('switches between methods when selecting a different builder', async () => {
    const { user } = setup();
    renderWithProviders(<RgbPatternPage />);

    await user.click(screen.getByRole('button', { name: /equation method/i }));

    expect(
      screen.getByRole('heading', { level: 3, name: /equation rgb pattern editor/i }),
    ).toBeInTheDocument();
    expect(
      screen.queryByRole('heading', { level: 3, name: /simple method/i }),
    ).not.toBeInTheDocument();
  });

  it('validates the pattern when switching methods', async () => {
    const { user } = setup();
    renderWithProviders(<RgbPatternPage />);

    // Switch to equation method (default state is invalid because it has no sections)
    await user.click(screen.getByRole('button', { name: /equation method/i }));

    expect(screen.getByText(/at least one equation section is required/i)).toBeInTheDocument();

    // Switch back to simple method (default state is valid)
    await user.click(screen.getByRole('button', { name: /simple method/i }));

    expect(
      screen.queryByText(/at least one equation section is required/i),
    ).not.toBeInTheDocument();
  });

  it('disables save button on initial load due to invalid default pattern', () => {
    setup();
    renderWithProviders(<RgbPatternPage />);

    const saveButton = screen.getByRole('button', { name: /save pattern/i });
    expect(saveButton).toBeDisabled();
  });

  it('disables save button when switching to new pattern', async () => {
    const { user } = setup();
    const storedPattern: SimplePattern = {
      type: 'simple',
      name: 'Valid Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
    };
    usePatternStore.getState().savePattern(storedPattern);

    renderWithProviders(<RgbPatternPage />);

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
    renderWithProviders(<RgbPatternPage />);

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

    const confirmSpy = vi.spyOn(window, 'confirm').mockImplementation(() => true);

    await user.click(saveButton);

    expect(confirmSpy).toHaveBeenCalledWith(expect.stringMatching(/replace it/i));
    confirmSpy.mockRestore();
  });

  it('shows validation errors when saving an invalid pattern', async () => {
    const { user } = setup();
    renderWithProviders(<RgbPatternPage />);

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
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
    };
    usePatternStore.getState().savePattern(storedPattern);

    renderWithProviders(<RgbPatternPage />);

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
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#123456') }],
    };
    usePatternStore.getState().savePattern(storedPattern);

    renderWithProviders(<RgbPatternPage />);

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
    renderWithProviders(<RgbPatternPage />);

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

  it('restores the selected pattern name when switching back to a method with a loaded pattern', async () => {
    const { user } = setup();
    const storedPattern: SimplePattern = {
      type: 'simple',
      name: 'My Simple Pattern',
      duration: 100,
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#ffffff') }],
    };
    usePatternStore.getState().savePattern(storedPattern);

    renderWithProviders(<RgbPatternPage />);

    // Select the pattern
    const chooser = screen.getByLabelText(/saved patterns/i);
    await user.selectOptions(chooser, storedPattern.name);
    expect(chooser).toHaveValue(storedPattern.name);

    // Switch to Equation
    await user.click(screen.getByRole('button', { name: /equation method/i }));
    expect(
      screen.getByRole('heading', { level: 3, name: /equation rgb pattern editor/i }),
    ).toBeInTheDocument();

    // Switch back to Simple
    await user.click(screen.getByRole('button', { name: /simple method/i }));

    // Verify the pattern is still selected in the dropdown
    expect(screen.getByLabelText(/saved patterns/i)).toHaveValue(storedPattern.name);
  });

  it('shows validation errors when saving an empty equation pattern', async () => {
    const { user } = setup();
    renderWithProviders(<RgbPatternPage />);

    // Switch to Equation method
    await user.click(screen.getByRole('button', { name: /equation method/i }));

    // Try to save immediately (default equation pattern is empty)
    const saveButton = screen.getByRole('button', { name: /save pattern/i });
    await user.click(saveButton);

    expect(screen.getByText(/at least one equation section is required/i)).toBeInTheDocument();
  });

  it('filters out binary patterns from the saved patterns list', () => {
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

    renderWithProviders(<RgbPatternPage />);

    const chooser = screen.getByLabelText(/saved patterns/i);

    // Check that Color Pattern is an option
    expect(within(chooser).getByRole('option', { name: 'Color Pattern' })).toBeInTheDocument();

    // Check that Binary Pattern is NOT an option
    expect(
      within(chooser).queryByRole('option', { name: 'Binary Pattern' }),
    ).not.toBeInTheDocument();
  });
});
