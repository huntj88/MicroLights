import { t } from 'i18next';
import type { ChangeEvent } from 'react';

interface PatternNameEditorProps {
  name: string;
  onChange: (name: string) => void;
}

export const PatternNameEditor = ({ name, onChange }: PatternNameEditorProps) => {
  const handleChange = (event: ChangeEvent<HTMLInputElement>) => {
    onChange(event.target.value);
  };

  return (
    <label className="flex flex-col gap-1 text-sm">
      <span className="font-bold theme-muted">{t('rgbPattern.simple.form.nameLabel')}</span>
      <input
        className="w-full rounded-xl bg-[rgb(var(--surface-raised)/0.5)] theme-border border px-3 py-2 text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none transition-colors"
        onChange={handleChange}
        placeholder={t('rgbPattern.simple.form.namePlaceholder')}
        type="text"
        value={name}
      />
      <span className="text-xs theme-muted">{t('rgbPattern.simple.form.nameHelper')}</span>
    </label>
  );
};
