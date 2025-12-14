import type { ReactNode } from 'react';

interface SectionProps {
  title?: string;
  actions?: ReactNode;
  children: ReactNode;
}

export const Section = ({ title, actions, children }: SectionProps) => {
  return (
    <section className={`bg-[rgb(var(--surface-raised)/0.3)] rounded-xl p-4 border theme-border`}>
      {(title ?? actions) && (
        <header className="flex items-center justify-between mb-4">
          {title && <h3 className="text-sm font-bold theme-muted">{title}</h3>}
          {actions && <div className="flex gap-2">{actions}</div>}
        </header>
      )}
      {children}
    </section>
  );
};
