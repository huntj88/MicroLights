import { useTranslation } from 'react-i18next';

import { useTheme } from '@/app/providers/theme-context';
import {
  ThemePreferenceSelector,
  type ThemePreferenceSelectorProps,
  type ThemePreferenceState,
} from '@/components/settings/ThemePreferenceSelector';

export const SettingsPage = () => {
  const { t } = useTranslation();
  const { preference, resolved, setPreference } = useTheme();

  const state: ThemePreferenceState = {
    mode: preference,
    resolved,
  };

  const handleThemeChange: ThemePreferenceSelectorProps['onChange'] = newState => {
    setPreference(newState.mode);
  };

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('settings.title')}</h2>
        <p className="theme-muted">{t('settings.subtitle')}</p>
      </header>
      <div className="theme-panel theme-border rounded-xl border p-6">
        <ThemePreferenceSelector onChange={handleThemeChange} value={state} />
      </div>
    </section>
  );
};
