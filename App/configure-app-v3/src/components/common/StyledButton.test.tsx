import { describe, expect, it } from 'vitest';

import { render, screen } from '@/test-utils/render-with-providers';

import { StyledButton } from './StyledButton';

describe('StyledButton', () => {
  it('renders children text', () => {
    render(<StyledButton>Click me</StyledButton>);
    expect(screen.getByText('Click me')).toBeInTheDocument();
  });

  it('renders as a button element with type="button" by default', () => {
    render(<StyledButton>Test</StyledButton>);
    const button = screen.getByRole('button', { name: 'Test' });
    expect(button).toHaveAttribute('type', 'button');
  });

  it('allows overriding type to submit', () => {
    render(<StyledButton type="submit">Submit</StyledButton>);
    expect(screen.getByRole('button', { name: 'Submit' })).toHaveAttribute('type', 'submit');
  });

  it('applies the secondary variant by default', () => {
    render(<StyledButton>Default</StyledButton>);
    const button = screen.getByRole('button');
    expect(button.className).toContain('theme-border');
  });

  it('applies the primary variant classes', () => {
    render(<StyledButton variant="primary">Primary</StyledButton>);
    const button = screen.getByRole('button');
    expect(button.className).toContain('bg-[rgb(var(--accent)');
    expect(button.className).toContain('shadow-sm');
  });

  it('applies the danger variant classes', () => {
    render(<StyledButton variant="danger">Delete</StyledButton>);
    const button = screen.getByRole('button');
    expect(button.className).toContain('text-red-500');
    expect(button.className).toContain('border-red-500');
  });

  it('applies the ghost variant classes', () => {
    render(<StyledButton variant="ghost">Ghost</StyledButton>);
    const button = screen.getByRole('button');
    expect(button.className).toContain('theme-muted');
  });

  it('applies the success variant classes', () => {
    render(<StyledButton variant="success">OK</StyledButton>);
    const button = screen.getByRole('button');
    expect(button.className).toContain('bg-[rgb(var(--success)');
  });

  it('applies the warning variant classes', () => {
    render(<StyledButton variant="warning">Warn</StyledButton>);
    const button = screen.getByRole('button');
    expect(button.className).toContain('bg-[rgb(var(--warning)');
  });

  describe('size prop', () => {
    it('applies md size by default', () => {
      render(<StyledButton>Medium</StyledButton>);
      const button = screen.getByRole('button');
      expect(button.className).toContain('px-4');
      expect(button.className).toContain('text-sm');
    });

    it('applies sm size classes', () => {
      render(<StyledButton size="sm">Small</StyledButton>);
      const button = screen.getByRole('button');
      expect(button.className).toContain('px-3');
      expect(button.className).toContain('text-xs');
    });

    it('applies lg size classes', () => {
      render(<StyledButton size="lg">Large</StyledButton>);
      const button = screen.getByRole('button');
      expect(button.className).toContain('px-6');
      expect(button.className).toContain('text-base');
    });

    it('includes min touch target size for all sizes', () => {
      const { rerender } = render(<StyledButton size="sm">S</StyledButton>);
      expect(screen.getByRole('button').className).toContain('min-h-[44px]');
      expect(screen.getByRole('button').className).toContain('min-w-[44px]');

      rerender(<StyledButton size="md">M</StyledButton>);
      expect(screen.getByRole('button').className).toContain('min-h-[44px]');

      rerender(<StyledButton size="lg">L</StyledButton>);
      expect(screen.getByRole('button').className).toContain('min-h-[44px]');
    });
  });

  it('includes active scale feedback', () => {
    render(<StyledButton>Tap</StyledButton>);
    expect(screen.getByRole('button').className).toContain('active:scale-[0.97]');
  });

  it('applies disabled styles when disabled', () => {
    render(<StyledButton disabled>Disabled</StyledButton>);
    const button = screen.getByRole('button');
    expect(button).toBeDisabled();
    expect(button.className).toContain('disabled:opacity-50');
  });

  it('merges custom className', () => {
    render(<StyledButton className="my-custom-class">Custom</StyledButton>);
    const button = screen.getByRole('button');
    expect(button.className).toContain('my-custom-class');
  });

  it('forwards additional HTML attributes', () => {
    render(
      <StyledButton data-testid="my-btn" aria-label="action">
        Go
      </StyledButton>,
    );
    expect(screen.getByTestId('my-btn')).toBeInTheDocument();
    expect(screen.getByLabelText('action')).toBeInTheDocument();
  });
});
