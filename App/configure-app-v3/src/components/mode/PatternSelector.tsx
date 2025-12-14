import { useTranslation } from 'react-i18next';

import { type ModePattern } from '../../app/models/mode';

interface Props {
  value?: string;
  onChange: (patternName: string) => void;
  patterns: ModePattern[];
  label: string;
}

export const PatternSelector = ({ value, onChange, patterns, label }: Props) => {
  const { t } = useTranslation();

  return (
    <div className="space-y-1">
      <label className="text-sm font-medium">{label}</label>
      <select
        className="theme-input w-full rounded-md border px-3 py-2"
        value={value ?? ''}
        onChange={e => {
          onChange(e.target.value);
        }}
      >
        <option value="">{t('modeEditor.selectPattern')}</option>
        {patterns.map(pattern => (
          <option key={pattern.name} value={pattern.name}>
            {pattern.name}
          </option>
        ))}
      </select>
      {!value && <p className="text-xs text-red-500">{t('modeEditor.noPattern')}</p>}
    </div>
  );
};
