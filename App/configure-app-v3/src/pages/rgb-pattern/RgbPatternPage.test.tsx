import userEvent from '@testing-library/user-event';
import { describe, expect, it, vi } from 'vitest';

import type { ModePattern } from '@/app/models/mode';
import { usePatternStore } from '@/app/providers/pattern-store';
import { renderWithProviders, screen, within } from '@/test-utils/render-with-providers';

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

    expect(screen.getByRole('heading', { level: 2, name: /equation rgb pattern editor/i })).toBeInTheDocument();
    expect(screen.queryByRole('heading', { level: 3, name: /simple method/i })).not.toBeInTheDocument();
  });

  it('prompts before overwriting an existing pattern when saving', async () => {
    const { user } = setup();
    renderWithProviders(<RgbPatternPage />);

    const saveButton = screen.getByRole('button', { name: /save pattern/i });
    const addButton = screen.getByRole('button', { name: /add color step/i });
    expect(saveButton).toBeDisabled();

    await user.click(addButton);
    await user.click(screen.getByRole('button', { name: /add step/i }));
    expect(saveButton).toBeEnabled();

    await user.click(saveButton);

    const chooser = screen.getByLabelText(/saved patterns/i);
    const options = within(chooser).getAllByRole('option');
    expect(options).toHaveLength(2);
    expect(chooser).toHaveValue('Simple RGB Pattern');

    const confirmSpy = vi.spyOn(window, 'confirm').mockImplementation(() => true);

    await user.click(saveButton);

    expect(confirmSpy).toHaveBeenCalledWith(expect.stringMatching(/replace it/i));
    confirmSpy.mockRestore();
  });

  it('deletes the selected pattern after confirmation', async () => {
    const { user } = setup();
    const storedPattern: ModePattern = {
      type: 'simple',
      name: 'Stored Pattern',
      duration: 100,
      changeAt: [
        { ms: 0, output: '#123456' as ModePattern['changeAt'][number]['output'] },
      ],
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

    const addButton = screen.getByRole('button', { name: /add color step/i });
    await user.click(addButton);
    await user.click(screen.getByRole('button', { name: /add step/i }));

    expect(screen.getByRole('heading', { name: /pattern steps/i })).toBeInTheDocument();

    const chooser = screen.getByLabelText(/saved patterns/i);
    await user.selectOptions(chooser, '');

    expect(chooser).toHaveValue('');
    expect(screen.getAllByText(/no colors have been added yet/i)).toHaveLength(2);
    expect(screen.getByRole('textbox', { name: /pattern name/i })).toHaveValue('Simple RGB Pattern');
  });
});
