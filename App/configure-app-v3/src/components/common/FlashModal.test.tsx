import { fireEvent, render, screen } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';

import { FlashModal } from './FlashModal';

describe('FlashModal', () => {
  const mockOnClose = vi.fn();
  const mockOnConfirm = vi.fn();

  it('should not render when isOpen is false', () => {
    render(<FlashModal isOpen={false} onClose={mockOnClose} onConfirm={mockOnConfirm} />);
    expect(screen.queryByText('common.actions.flash')).not.toBeInTheDocument();
  });

  it('should render when isOpen is true', () => {
    render(<FlashModal isOpen={true} onClose={mockOnClose} onConfirm={mockOnConfirm} />);
    expect(screen.getAllByText('common.actions.flash')[0]).toBeInTheDocument();
    expect(screen.getByText('common.labels.index')).toBeInTheDocument();
  });

  it('should call onClose when cancel button is clicked', () => {
    render(<FlashModal isOpen={true} onClose={mockOnClose} onConfirm={mockOnConfirm} />);
    fireEvent.click(screen.getByText('common.actions.cancel'));
    expect(mockOnClose).toHaveBeenCalled();
  });

  it('should call onConfirm with selected index when flash button is clicked', () => {
    render(<FlashModal isOpen={true} onClose={mockOnClose} onConfirm={mockOnConfirm} />);
    
    // Default index is 0
    fireEvent.click(screen.getAllByText('common.actions.flash')[1]); // The second one is the button
    expect(mockOnConfirm).toHaveBeenCalledWith(0);
  });

  it('should allow selecting a different index', () => {
    render(<FlashModal isOpen={true} onClose={mockOnClose} onConfirm={mockOnConfirm} />);
    
    // Click button "3" (index 2)
    fireEvent.click(screen.getByText('3'));
    
    fireEvent.click(screen.getAllByText('common.actions.flash')[1]);
    expect(mockOnConfirm).toHaveBeenCalledWith(2);
  });
});
