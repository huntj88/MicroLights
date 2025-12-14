import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';

import { SectionLane } from './SectionLane';
import type { EquationSection } from '../../../../app/models/mode';

describe('SectionLane', () => {
  const mockSections: EquationSection[] = [{ equation: 'sin(t)', duration: 1000 }];

  const defaultProps = {
    color: 'red' as const,
    sections: mockSections,
    loopAfterDuration: true,
    onToggleLoop: vi.fn(),
    onAddSection: vi.fn(),
    onUpdateSection: vi.fn(),
    onDeleteSection: vi.fn(),
    onMoveSection: vi.fn(),
  };

  it('renders sections correctly', () => {
    render(<SectionLane {...defaultProps} />);
    expect(screen.getByDisplayValue('sin(t)')).toBeInTheDocument();
    expect(screen.getByDisplayValue('1000')).toBeInTheDocument();
  });

  it('allows clearing the duration input', () => {
    const onUpdateSection = vi.fn();
    render(<SectionLane {...defaultProps} onUpdateSection={onUpdateSection} />);

    const durationInput = screen.getByDisplayValue('1000');
    fireEvent.change(durationInput, { target: { value: '' } });

    expect(onUpdateSection).toHaveBeenCalledWith(0, { duration: NaN });
  });

  it('handles NaN duration gracefully', () => {
    const sectionsWithNaN: EquationSection[] = [{ equation: 'sin(t)', duration: NaN }];
    render(<SectionLane {...defaultProps} sections={sectionsWithNaN} />);

    const durationInput = screen.getByRole('spinbutton');
    // When value is NaN, input should be empty
    expect(durationInput).toHaveValue(null);
  });
});
