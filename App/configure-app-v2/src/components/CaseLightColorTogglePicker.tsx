import { useTranslation } from 'react-i18next';

import { DISABLED_COLOR } from '@/lib/constants';

export function CaseLightColorTogglePicker({
  color,
  onToggle,
  onChange,
  className,
  checkboxClassName = 'accent-fg-ring',
  colorInputClassName = 'h-8 w-12 p-0 bg-transparent border border-slate-700/50 rounded disabled:opacity-50',
}: {
  color: string;
  onToggle: (enabled: boolean) => void;
  onChange: (hex: string) => void;
  className?: string;
  checkboxClassName?: string;
  colorInputClassName?: string;
}) {
  const { t } = useTranslation();
  const enabled = color !== DISABLED_COLOR;

  return (
    <div className={`flex items-center gap-3 ${className ?? ''}`}>
      <input
        type="checkbox"
        aria-label={t('enableCaseLight')}
        title={t('enableCaseLight')}
        checked={enabled}
        onChange={e => onToggle(e.target.checked)}
        className={checkboxClassName}
      />
      <input
        type="color"
        value={color}
        onChange={e => onChange(e.target.value)}
        disabled={!enabled}
        className={colorInputClassName}
        aria-label={t('caseLightColorAria')}
        title={enabled ? t('caseLightColor') : t('enableCaseLightToChoose')}
      />
    </div>
  );
}
