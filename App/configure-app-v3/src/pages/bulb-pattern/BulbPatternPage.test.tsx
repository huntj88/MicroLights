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
      screen.getByRole('heading', { level: 2, name: 'bulbPattern.title' }),
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

    const chooser = screen.getByLabelText('patternEditor.storage.selectLabel');

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

    const saveButton = screen.getByRole('button', { name: 'patternEditor.storage.saveButton' });
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
    const chooser = screen.getByLabelText('patternEditor.storage.selectLabel');
    await user.selectOptions(chooser, storedPattern.name);

    // Switch to New Pattern
    await user.selectOptions(chooser, '');

    const saveButton = screen.getByRole('button', { name: 'patternEditor.storage.saveButton' });
    expect(saveButton).toBeDisabled();
  });

  it('does not prompt when updating the currently selected pattern', async () => {
    const { user } = setup();
    renderWithProviders(<BulbPatternPage />);

    const saveButton = screen.getByRole('button', { name: 'patternEditor.storage.saveButton' });
    const addButton = screen.getByRole('button', { name: 'patternEditor.form.addButton' });
    const nameInput = screen.getByRole('textbox', { name: /patternEditor.form.nameLabel/i });

    await user.type(nameInput, 'My Pattern');
    await user.click(addButton);

    let dialog = await screen.findByRole('dialog');
    await user.click(
      within(dialog).getByRole('button', { name: 'patternEditor.addModal.confirm' }),
    );
    expect(saveButton).toBeEnabled();

    await user.click(saveButton);

    const chooser = screen.getByLabelText('patternEditor.storage.selectLabel');
    const options = within(chooser).getAllByRole('option');
    expect(options).toHaveLength(2);
    expect(chooser).toHaveValue('My Pattern');

    // Make a change to trigger the overwrite prompt (otherwise save is disabled)
    await user.click(addButton);

    dialog = await screen.findByRole('dialog');
    await user.click(
      within(dialog).getByRole('button', { name: 'patternEditor.addModal.confirm' }),
    );

    const confirmSpy = vi.spyOn(window, 'confirm').mockImplementation(() => true);

    await user.click(saveButton);

    expect(confirmSpy).not.toHaveBeenCalled();
    confirmSpy.mockRestore();
  });

  it('prompts before overwriting a different existing pattern', async () => {
    const { user } = setup();
    const patternA: SimplePattern = {
      type: 'simple',
      name: 'Pattern A',
      duration: 100,
      changeAt: [{ ms: 0, output: 'high' }],
    };
    const patternB: SimplePattern = {
      type: 'simple',
      name: 'Pattern B',
      duration: 100,
      changeAt: [{ ms: 0, output: 'low' }],
    };
    usePatternStore.getState().savePattern(patternA);
    usePatternStore.getState().savePattern(patternB);

    renderWithProviders(<BulbPatternPage />);

    // Select Pattern B
    const chooser = screen.getByLabelText('patternEditor.storage.selectLabel');
    await user.selectOptions(chooser, 'Pattern B');

    // Rename to Pattern A
    const nameInput = screen.getByRole('textbox', { name: /patternEditor.form.nameLabel/i });
    await user.clear(nameInput);
    await user.type(nameInput, 'Pattern A');

    const saveButton = screen.getByRole('button', { name: 'patternEditor.storage.saveButton' });

    const confirmSpy = vi.spyOn(window, 'confirm').mockImplementation(() => true);

    await user.click(saveButton);

    expect(confirmSpy).toHaveBeenCalledWith(
      expect.stringMatching('patternEditor.storage.overwriteConfirm'),
    );
    confirmSpy.mockRestore();
  });

  it('shows validation errors when saving an invalid pattern', async () => {
    const { user } = setup();
    renderWithProviders(<BulbPatternPage />);

    const saveButton = screen.getByRole('button', { name: 'patternEditor.storage.saveButton' });
    await user.click(saveButton);

    expect(screen.getByText('validation.pattern.simple.changeEventRequired')).toBeInTheDocument();
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

    const chooser = screen.getByLabelText('patternEditor.storage.selectLabel');
    await user.selectOptions(chooser, storedPattern.name);

    const saveButton = screen.getByRole('button', { name: 'patternEditor.storage.saveButton' });
    await waitFor(() => {
      expect(saveButton).toBeDisabled();
    });

    // Change name
    const nameInput = screen.getByRole('textbox', { name: /patternEditor.form.nameLabel/i });
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

    const chooser = screen.getByLabelText('patternEditor.storage.selectLabel');
    await user.selectOptions(chooser, storedPattern.name);
    expect(screen.getByRole('textbox', { name: /patternEditor.form.nameLabel/i })).toHaveValue(
      storedPattern.name,
    );

    const confirmSpy = vi.spyOn(window, 'confirm').mockImplementation(() => true);

    await user.click(screen.getByRole('button', { name: 'patternEditor.storage.deleteButton' }));

    expect(confirmSpy).toHaveBeenCalledWith(
      expect.stringMatching('patternEditor.storage.deleteConfirm'),
    );
    confirmSpy.mockRestore();

    expect(usePatternStore.getState().patterns).toHaveLength(0);
    expect(chooser).toHaveValue('');
    expect(screen.getByRole('textbox', { name: /patternEditor.form.nameLabel/i })).toHaveValue('');
  });

  it('resets the builder when switching back to the new pattern option', async () => {
    const { user } = setup();
    renderWithProviders(<BulbPatternPage />);

    const addButton = screen.getByRole('button', { name: 'patternEditor.form.addButton' });
    await user.click(addButton);

    const dialog = await screen.findByRole('dialog');
    await user.click(
      within(dialog).getByRole('button', { name: 'patternEditor.addModal.confirm' }),
    );

    expect(
      screen.getByRole('heading', { name: 'patternEditor.steps.title #1' }),
    ).toBeInTheDocument();

    const chooser = screen.getByLabelText('patternEditor.storage.selectLabel');
    await user.selectOptions(chooser, '');

    expect(chooser).toHaveValue('');
    expect(screen.getAllByText('patternEditor.preview.empty')).toHaveLength(2);
    expect(screen.getByRole('textbox', { name: /patternEditor.form.nameLabel/i })).toHaveValue('');
  });
});
