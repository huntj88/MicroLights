import { useAppStore } from '@/lib/store';

export default function Settings() {
  const pref = useAppStore(s => s.theme);
  const setPref = useAppStore(s => s.setThemePreference);

  return (
    <div className="space-y-6">
      <h1 className="text-2xl font-semibold">Settings</h1>

      <section className="space-y-2">
        <div className="text-xs uppercase tracking-wide text-slate-400">Appearance</div>
        <div className="grid grid-cols-[140px_1fr] items-center gap-3">
          <div className="text-sm text-fg-muted">Theme</div>
          <div className="flex gap-3">
            <label className="flex items-center gap-2 text-sm">
              <input
                type="radio"
                name="theme"
                value="system"
                checked={pref === 'system'}
                onChange={() => setPref('system')}
              />
              System
            </label>
            <label className="flex items-center gap-2 text-sm">
              <input
                type="radio"
                name="theme"
                value="light"
                checked={pref === 'light'}
                onChange={() => setPref('light')}
              />
              Light
            </label>
            <label className="flex items-center gap-2 text-sm">
              <input
                type="radio"
                name="theme"
                value="dark"
                checked={pref === 'dark'}
                onChange={() => setPref('dark')}
              />
              Dark
            </label>
          </div>
          <div className="col-span-2 text-xs text-slate-400">
            When set to System, the app follows your OS setting (prefers-color-scheme).
          </div>
        </div>
      </section>
    </div>
  );
}
