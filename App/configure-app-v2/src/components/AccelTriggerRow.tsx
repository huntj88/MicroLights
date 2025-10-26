import { useTranslation } from 'react-i18next';

import { CaseLightColorTogglePicker } from '@/components/CaseLightColorTogglePicker';
import { CloseButton } from '@/components/CloseButton';
import { ALLOWED_THRESHOLDS } from '@/lib/store';
import type { BulbModeWaveformDoc, Trigger } from '@/lib/store';

import { BulbModeWaveformPicker } from '@/components/BulbModeWaveformPicker';

export function AccelTriggerRow({
  trigger,
  index,
  prevThreshold,
  waveforms,
  defaultColor,
  onChangeThreshold,
  onRemove,
  onChangeWaveform,
  onCreateWaveform,
  onEditWaveform,
  onToggleColor,
  onColorChange,
}: {
  trigger: Trigger;
  index: number;
  prevThreshold?: number;
  waveforms: BulbModeWaveformDoc[];
  defaultColor: string;
  onChangeThreshold: (v: number) => void;
  onRemove: () => void;
  onChangeWaveform: (id: string | undefined) => void;
  onCreateWaveform: () => void;
  onEditWaveform: (id: string) => void;
  onToggleColor: (enabled: boolean) => void;
  onColorChange: (hex: string) => void;
}) {
  const { t } = useTranslation();
  const allowedAfterPrev =
    prevThreshold == null
      ? [...ALLOWED_THRESHOLDS]
      : (ALLOWED_THRESHOLDS.filter(v => v > prevThreshold) as number[]);

  const effectiveColor = trigger.color ?? defaultColor;

  return (
    <div className="rounded-lg border border-slate-700/50 bg-slate-900/40 p-3 pl-4 relative">
      {/* Top-right close button to remove this trigger */}
      <CloseButton className="absolute top-2 right-2" onClick={onRemove} />
      <div className="absolute left-0 top-0 bottom-0 w-1 rounded-l-lg bg-fg-ring/40" />
      <div className="mb-2 text-[11px] uppercase tracking-wide text-slate-400">
        {t('triggerWithIndex', { index: index + 1 })}
      </div>
      <div className="grid grid-cols-[auto_1fr_auto] items-center gap-2">
        <div className="text-[11px] uppercase tracking-wide text-slate-400">{t('thresholdXg')}</div>
        <select
          value={trigger.threshold}
          onChange={e => onChangeThreshold(Number(e.target.value))}
          className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
          aria-label={t('accelerationThresholdAria')}
        >
          {allowedAfterPrev.map(v => (
            <option key={v} value={v}>
              {t('accelGValue', { value: v })}
            </option>
          ))}
        </select>
        {/* spacer to keep grid alignment since close button is absolutely positioned */}
        <div />

        <div className="text-[11px] uppercase tracking-wide text-slate-400 self-start mt-1.5">
          {t('waveform')}
        </div>
        <div className="flex items-center gap-2 self-start">
          <BulbModeWaveformPicker
            value={trigger.bulbModeWaveformId}
            onChange={onChangeWaveform}
            waveforms={waveforms}
            onCreate={onCreateWaveform}
            onEdit={onEditWaveform}
          />
        </div>

        <CaseLightColorTogglePicker
          color={effectiveColor}
          onToggle={onToggleColor}
          onChange={onColorChange}
          className="justify-self-end ml-3 sm:ml-4 self-start"
        />
      </div>
    </div>
  );
}
