import { useTranslation } from 'react-i18next';

import { useAppStore } from '@/lib/store';

export default function Settings() {
  const { t } = useTranslation();
  const pref = useAppStore(s => s.theme);
  const setPref = useAppStore(s => s.setThemePreference);

  return (
    <div className="space-y-6">
      <h1 className="text-2xl font-semibold">{t('settings')}</h1>

      <section className="space-y-2">
        <div className="text-xs uppercase tracking-wide text-slate-400">{t('appearance')}</div>
        <div className="grid grid-cols-[140px_1fr] items-center gap-3">
          <div className="text-sm text-fg-muted">{t('theme')}</div>
          <div className="flex gap-3">
            <label className="flex items-center gap-2 text-sm">
              <input
                type="radio"
                name="theme"
                value="system"
                checked={pref === 'system'}
                onChange={() => setPref('system')}
              />
              {t('system')}
            </label>
            <label className="flex items-center gap-2 text-sm">
              <input
                type="radio"
                name="theme"
                value="light"
                checked={pref === 'light'}
                onChange={() => setPref('light')}
              />
              {t('light')}
            </label>
            <label className="flex items-center gap-2 text-sm">
              <input
                type="radio"
                name="theme"
                value="dark"
                checked={pref === 'dark'}
                onChange={() => setPref('dark')}
              />
              {t('dark')}
            </label>
          </div>
          <div className="col-span-2 text-xs text-slate-400">
            {t('systemThemeHint')}
          </div>
        </div>
      </section>
    </div>
  );
}
