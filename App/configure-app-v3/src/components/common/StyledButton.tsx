import type { ButtonHTMLAttributes, ReactNode } from 'react';

interface StyledButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  children: ReactNode;
  variant?: 'primary' | 'secondary' | 'danger' | 'ghost' | 'success' | 'warning';
  size?: 'sm' | 'md' | 'lg';
}

export const StyledButton = ({
  children,
  variant = 'secondary',
  size = 'md',
  className = '',
  ...props
}: StyledButtonProps) => {
  const baseStyles =
    'rounded-full font-bold transition-all disabled:opacity-50 disabled:cursor-not-allowed flex items-center justify-center gap-2 active:scale-[0.97]';

  const sizeStyles = {
    sm: 'px-3 py-2 text-xs min-h-[44px] min-w-[44px]',
    md: 'px-4 py-2.5 text-sm min-h-[44px] min-w-[44px]',
    lg: 'px-6 py-3 text-base min-h-[44px] min-w-[44px]',
  };

  const variants = {
    primary:
      'bg-[rgb(var(--accent)/1)] hover:opacity-90 text-[rgb(var(--surface-contrast)/1)] shadow-sm hover:shadow',
    secondary:
      'bg-[rgb(var(--surface-raised)/1)] hover:bg-[rgb(var(--surface-raised)/0.8)] text-[rgb(var(--surface-contrast)/1)] border theme-border',
    danger: 'bg-red-500/10 text-red-500 hover:bg-red-500/20 border border-red-500/20',
    ghost:
      'hover:bg-[rgb(var(--surface-raised)/0.5)] theme-muted hover:text-[rgb(var(--surface-contrast)/1)]',
    success: 'bg-[rgb(var(--success)/1)] hover:bg-[rgb(var(--success)/0.9)] text-white shadow-sm',
    warning:
      'bg-[rgb(var(--warning)/1)] hover:bg-[rgb(var(--warning)/0.9)] text-gray-900 shadow-sm',
  };

  return (
    <button
      className={`${baseStyles} ${sizeStyles[size]} ${variants[variant]} ${className}`}
      type="button"
      {...props}
    >
      {children}
    </button>
  );
};
