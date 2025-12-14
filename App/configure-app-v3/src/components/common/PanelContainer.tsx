import type { ReactNode } from 'react';

interface PanelContainerProps {
  children: ReactNode;
  className?: string;
}

export const PanelContainer = ({ children, className = '' }: PanelContainerProps) => {
  return (
    <div
      className={`flex flex-col gap-6 p-4 theme-panel theme-border border rounded-xl shadow-sm text-[rgb(var(--surface-contrast)/1)] ${className}`}
    >
      {children}
    </div>
  );
};
