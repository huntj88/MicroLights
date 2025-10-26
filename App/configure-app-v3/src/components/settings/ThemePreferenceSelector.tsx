import type { ChangeEvent } from 'react';
import { useTranslation } from 'react-i18next';

import type { ThemePreference, ThemeValue } from '@/app/providers/theme-store';

export interface ThemePreferenceState {
  mode: ThemePreference;
  resolved: ThemeValue;
}

export interface ThemePreferenceAction {
  type: 'change-mode';
  mode: ThemePreference;
}

export interface ThemePreferenceSelectorProps {
  value: ThemePreferenceState;
  onChange: (newState: ThemePreferenceState, action: ThemePreferenceAction) => void;
}

const themeOptions: { key: ThemePreference; translationKey: string; descriptionKey: string }[] = [
  { key: 'system', translationKey: 'settings.theme.options.system.label', descriptionKey: 'settings.theme.options.system.description' },
  { key: 'light', translationKey: 'settings.theme.options.light.label', descriptionKey: 'settings.theme.options.light.description' },
  { key: 'dark', translationKey: 'settings.theme.options.dark.label', descriptionKey: 'settings.theme.options.dark.description' },
];

export const ThemePreferenceSelector = ({ value, onChange }: ThemePreferenceSelectorProps) => {
  const { t } = useTranslation();

  const handleChange = (event: ChangeEvent<HTMLInputElement>) => {
    const nextMode = event.target.value as ThemePreference;

    if (nextMode === value.mode) {
      return;
    }

    const newState: ThemePreferenceState = {
      ...value,
      mode: nextMode,
    };

    onChange(newState, {
      type: 'change-mode',
      mode: nextMode,
    });
  };

  return (
    <fieldset className="space-y-4">
      <legend className="text-lg font-semibold">{t('settings.theme.title')}</legend>
      <p className="theme-muted text-sm">
        {t('settings.theme.description', { theme: t(`settings.theme.current.${value.resolved}`) })}
      </p>
      <div className="grid gap-3 md:grid-cols-3">
        {themeOptions.map(option => {
          const label = t(option.translationKey);
          const description = t(option.descriptionKey);
          const inputId = `theme-preference-${option.key}`;

          return (
            <label
              key={option.key}
              className="theme-panel theme-border flex cursor-pointer flex-col gap-2 rounded-xl border p-4 transition-colors hover:border-[rgb(var(--accent)/1)]"
              htmlFor={inputId}
            >
              <span className="flex items-center justify-between">
                <span className="font-medium capitalize">{label}</span>
                <input
                  aria-label={label}
                  checked={value.mode === option.key}
                  className="size-4 accent-[rgb(var(--accent)/1)]"
                  id={inputId}
                  name="theme-preference"
                  onChange={handleChange}
                  type="radio"
                  value={option.key}
                />
              </span>
              <span className="text-sm theme-muted">{description}</span>
            </label>
          );
        })}
      </div>
    </fieldset>
  );
};
