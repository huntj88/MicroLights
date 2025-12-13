import type { ButtonHTMLAttributes, ReactNode } from 'react';

interface PatternButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  children: ReactNode;
  variant?: 'primary' | 'secondary' | 'danger' | 'ghost' | 'success' | 'warning';
}

export const PatternButton = ({
  children,
  variant = 'secondary',
  className = '',
  ...props
}: PatternButtonProps) => {
  const baseStyles =
    'px-4 py-2 rounded-full font-bold transition-all disabled:opacity-50 disabled:cursor-not-allowed flex items-center justify-center gap-2 text-sm';

  const variants = {
    primary:
      'bg-[rgb(var(--accent)/1)] hover:opacity-90 text-[rgb(var(--surface-contrast)/1)] shadow-sm hover:shadow',
    secondary:
      'bg-[rgb(var(--surface-raised)/1)] hover:bg-[rgb(var(--surface-raised)/0.8)] text-[rgb(var(--surface-contrast)/1)] border theme-border',
    danger: 'bg-red-500/10 text-red-500 hover:bg-red-500/20 border border-red-500/20',
    ghost:
      'hover:bg-[rgb(var(--surface-raised)/0.5)] theme-muted hover:text-[rgb(var(--surface-contrast)/1)]',
    success: 'bg-green-600 hover:bg-green-500 text-white shadow-sm',
    warning: 'bg-yellow-500 hover:bg-yellow-400 text-gray-900 shadow-sm',
  };

  return (
    <button className={`${baseStyles} ${variants[variant]} ${className}`} type="button" {...props}>
      {children}
    </button>
  );
};
