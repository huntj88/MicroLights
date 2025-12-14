import { useTranslation } from 'react-i18next';

import { PatternPreview } from './previews/PatternPreview';
import { type ModePattern } from '../../app/models/mode';

interface Props {
  value?: string;
  onChange: (patternName: string) => void;
  patterns: ModePattern[];
  label: string;
}

export const PatternSelector = ({ value, onChange, patterns, label }: Props) => {
  const { t } = useTranslation();
  const selectedPattern = patterns.find(p => p.name === value);

  return (
    <div className="space-y-1">
      <label className="text-sm font-medium theme-muted">{label}</label>
      <select
        className="w-full rounded-xl bg-[rgb(var(--surface-raised)/0.5)] theme-border border px-3 py-2 text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none transition-colors"
        value={value ?? ''}
        onChange={e => {
          onChange(e.target.value);
        }}
      >
        <option value="">{t('modeEditor.storage.selectPattern')}</option>
        {patterns.map(pattern => (
          <option key={pattern.name} value={pattern.name}>
            {pattern.name}
          </option>
        ))}
      </select>
      {selectedPattern && (
        <div className="mt-2">
          <PatternPreview pattern={selectedPattern} />
        </div>
      )}
    </div>
  );
};
