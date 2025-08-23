import { Link, NavLink, Outlet } from 'react-router-dom';
import { Toaster } from 'sonner';

export function AppShell() {
  return (
    <div className="min-h-screen bg-bg text-fg">
      <div className="flex">
        <aside className="w-64 bg-bg-card border-r border-slate-700/50 min-h-screen p-4">
          <div className="font-semibold text-lg mb-4">
            <Link to="/">BulbChips</Link>
          </div>
          <nav className="space-y-1 text-fg-muted">
            <Section title="Create">
              <SidebarLink to="/create/set">Set</SidebarLink>
              <SidebarLink to="/create/wave">Wave</SidebarLink>
            </Section>
            <Section title="Browse">
              <SidebarLink to="/browse">Browse</SidebarLink>
            </Section>
            <Section title="Program">
              <SidebarLink to="/program">Program</SidebarLink>
            </Section>
            <Section title="Extra">
              <SidebarLink to="/theme">Theme</SidebarLink>
              <SidebarLink to="/docs">Documentation</SidebarLink>
              <SidebarLink to="/examples">Examples</SidebarLink>
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
