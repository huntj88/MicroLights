import { useEffect } from 'react';
import { useTranslation } from 'react-i18next';
import { Link, NavLink, Outlet } from 'react-router-dom';
import { Toaster } from 'sonner';

import { useAppStore } from '@/lib/store';

export function AppShell() {
  const { t } = useTranslation();
  const pref = useAppStore(s => s.theme);

  useEffect(() => {
    const mq = window.matchMedia('(prefers-color-scheme: dark)');
    const apply = () => {
      const wantsDark = pref === 'dark' || (pref === 'system' && mq.matches);
      document.documentElement.classList.toggle('dark', wantsDark);
    };
    apply();
    const onChange = () => apply();
    mq.addEventListener?.('change', onChange);
    return () => mq.removeEventListener?.('change', onChange);
  }, [pref]);

  return (
    <div className="min-h-screen bg-bg text-fg">
      <div className="flex">
        <aside className="w-64 bg-bg-card border-r border-slate-700/50 min-h-screen p-4">
          <div className="font-semibold text-lg mb-4">
            <Link to="/">{t('appName')}</Link>
          </div>
          <nav className="space-y-1 text-fg-muted">
            <Section title={t('create')}>
              <SidebarLink to="/create/mode">{t('mode')}</SidebarLink>
              <SidebarLink to="/create/wave">{t('wave')}</SidebarLink>
            </Section>
            <Section title={t('browse')}>
              <SidebarLink to="/browse">{t('browse')}</SidebarLink>
            </Section>
            <Section title={t('program')}>
              <SidebarLink to="/program">{t('program')}</SidebarLink>
            </Section>
            <Section title={t('extra')}>
              <SidebarLink to="/settings">{t('settings')}</SidebarLink>
              <SidebarLink to="/docs">{t('docs')}</SidebarLink>
              <SidebarLink to="/examples">{t('examples')}</SidebarLink>
            </Section>
          </nav>
        </aside>
        <main className="flex-1 p-6">
          <Outlet />
        </main>
      </div>
      <Toaster richColors />
    </div>
  );
}

function Section({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <div className="mb-4">
      <div className="uppercase text-xs tracking-wide mb-2 text-slate-400">{title}</div>
      <div className="flex flex-col space-y-1">{children}</div>
    </div>
  );
}

function SidebarLink({ to, children }: { to: string; children: React.ReactNode }) {
  return (
    <NavLink
      to={to}
      className={({ isActive }: { isActive: boolean }) =>
        `px-2 py-1 rounded hover:bg-slate-700/40 ${isActive ? 'bg-slate-700/60 text-white' : ''}`
      }
    >
      {children}
    </NavLink>
  );
}
