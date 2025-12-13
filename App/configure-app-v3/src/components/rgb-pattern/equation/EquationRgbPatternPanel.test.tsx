import userEvent from '@testing-library/user-event';
import { useState } from 'react';
import { describe, expect, it, vi } from 'vitest';

import { createDefaultEquationPattern } from '@/app/models/mode';
import { renderWithProviders, screen, within } from '@/test-utils/render-with-providers';

import {
  EquationRgbPatternPanel,
  type EquationRgbPatternPanelProps,
} from './EquationRgbPatternPanel';


const renderComponent = (props?: Partial<EquationRgbPatternPanelProps>) => {
  const defaultPattern = createDefaultEquationPattern();
  defaultPattern.name = 'Test Pattern';
  
  return renderWithProviders(
    <EquationRgbPatternPanel
      onChange={vi.fn()}
      pattern={defaultPattern}
      {...props}
    />,
  );
};

describe('EquationRgbPatternPanel', () => {
  it('renders the pattern name input', () => {
    renderComponent();
    expect(screen.getByRole('textbox', { name: /pattern name/i })).toHaveValue('Test Pattern');
  });

  it('emits rename-pattern action when name is changed', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    
    const Harness = () => {
      const [pattern, setPattern] = useState(() => {
        const p = createDefaultEquationPattern();
        p.name = 'Test Pattern';
        return p;
      });
      return (
        <EquationRgbPatternPanel
          onChange={(nextPattern, action) => {
            setPattern(nextPattern);
            handleChange(nextPattern, action);
          }}
          pattern={pattern}
        />
      );
    };

    renderWithProviders(<Harness />);

    const nameInput = screen.getByRole('textbox', { name: /pattern name/i });
    await user.clear(nameInput);
    await user.type(nameInput, 'New Name');

    expect(handleChange).toHaveBeenCalled();
    const lastCall = handleChange.mock.calls.at(-1) as Parameters<EquationRgbPatternPanelProps['onChange']>;
    const [nextPattern, action] = lastCall;

    expect(nextPattern.name).toBe('New Name');
    expect(action).toEqual({ type: 'rename-pattern', name: 'New Name' });
  });

  it('adds a section to the red channel', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    
    renderComponent({ onChange: handleChange });

    const redSectionHeader = screen.getByText(/red sections/i);
    const redContainer = redSectionHeader.closest('div')?.parentElement;
    if (!redContainer) throw new Error('Red container not found');

    const addButton = within(redContainer).getByRole('button', { name: /\+ add section/i });
    await user.click(addButton);

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<EquationRgbPatternPanelProps['onChange']>;

    expect(nextPattern.red.sections).toHaveLength(1);
    expect(action.type).toBe('add-section');
    if (action.type === 'add-section') {
      expect(action.channel).toBe('red');
    }
  });

  it('updates a section equation', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createDefaultEquationPattern();
    pattern.red.sections = [{ id: 's1', equation: '0', duration: 1000 }];

    const Harness = () => {
      const [p, setP] = useState(pattern);
      return (
        <EquationRgbPatternPanel
          onChange={(next, action) => {
            setP(next);
            handleChange(next, action);
          }}
          pattern={p}
        />
      );
    };

    renderWithProviders(<Harness />);

    const redSectionHeader = screen.getByText(/red sections/i);
    const redContainer = redSectionHeader.closest('div')?.parentElement;
    if (!redContainer) throw new Error('Red container not found');

    const equationInput = within(redContainer).getByRole('textbox');
    await user.clear(equationInput);
    await user.type(equationInput, 'sin(t)');

    expect(handleChange).toHaveBeenCalled();
    const lastCall = handleChange.mock.calls.at(-1) as Parameters<EquationRgbPatternPanelProps['onChange']>;
    const [nextPattern, action] = lastCall;

    expect(nextPattern.red.sections[0].equation).toBe('sin(t)');
    expect(action.type).toBe('update-section');
    if (action.type === 'update-section') {
      expect(action.sectionId).toBe('s1');
      expect(action.section.equation).toBe('sin(t)');
    }
  });

  it('removes a section', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createDefaultEquationPattern();
    pattern.red.sections = [{ id: 's1', equation: '0', duration: 1000 }];

    renderComponent({ onChange: handleChange, pattern });

    const deleteButton = screen.getByRole('button', { name: /delete/i });
    await user.click(deleteButton);

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<EquationRgbPatternPanelProps['onChange']>;

    expect(nextPattern.red.sections).toHaveLength(0);
    expect(action).toEqual({ type: 'remove-section', channel: 'red', sectionId: 's1' });
  });

  it('moves a section up', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createDefaultEquationPattern();
    pattern.red.sections = [
      { id: 's1', equation: '1', duration: 1000 },
      { id: 's2', equation: '2', duration: 1000 },
    ];

    renderComponent({ onChange: handleChange, pattern });

    const moveUpButtons = screen.getAllByTitle(/move up/i);
    // The first section's move up should be disabled or not present.
    // We want to click the second one (index 1).
    
    await user.click(moveUpButtons[1]);

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<EquationRgbPatternPanelProps['onChange']>;

    expect(nextPattern.red.sections[0].id).toBe('s2');
    expect(nextPattern.red.sections[1].id).toBe('s1');
    expect(action).toEqual({ type: 'move-section', channel: 'red', fromIndex: 1, toIndex: 0 });
  });

  it('toggles loop option for a channel', async () => {
    const handleChange = vi.fn();
    const user = userEvent.setup();
    const pattern = createDefaultEquationPattern();
    // Default is true
    expect(pattern.red.loopAfterDuration).toBe(true);

    renderComponent({ onChange: handleChange, pattern });

    const redSectionHeader = screen.getByText(/red sections/i);
    const redContainer = redSectionHeader.closest('div')?.parentElement;
    if (!redContainer) throw new Error('Red container not found');

    const loopCheckbox = within(redContainer).getByRole('checkbox', { name: /loop/i });
    await user.click(loopCheckbox);

    expect(handleChange).toHaveBeenCalledTimes(1);
    const [nextPattern, action] = handleChange.mock.calls[0] as Parameters<EquationRgbPatternPanelProps['onChange']>;

    expect(nextPattern.red.loopAfterDuration).toBe(false);
    expect(action).toEqual({ type: 'update-channel-config', channel: 'red', loopAfterDuration: false });
  });
});
