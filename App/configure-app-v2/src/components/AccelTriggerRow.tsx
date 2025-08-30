import { useTranslation } from 'react-i18next';

import { CaseLightColorTogglePicker } from '@/components/CaseLightColorTogglePicker';
import { WaveformMini } from '@/components/WaveformMini';
import { WaveformPicker } from '@/components/WaveformPicker';
import { ALLOWED_THRESHOLDS } from '@/lib/store';
import type { Trigger } from '@/lib/store';
import type { Waveform } from '@/lib/waveform';

type WaveformDoc = { id: string; readonly?: boolean } & Waveform;

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
  waveforms: WaveformDoc[];
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

  const selected = trigger.waveformId
    ? (waveforms.find(w => w.id === trigger.waveformId) ?? null)
    : null;

  const effectiveColor = trigger.color ?? defaultColor;

  return (
    <div className="rounded-lg border border-slate-700/50 bg-slate-900/40 p-3 pl-4 relative">
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
        <button
          className="px-2 py-1 rounded border border-red-600/40 text-red-400 hover:bg-red-600/10 text-xs"
          onClick={onRemove}
        >
          {t('remove')}
        </button>

        <div className="text-[11px] uppercase tracking-wide text-slate-400">{t('waveform')}</div>
        <div className="flex items-center gap-2">
          <WaveformPicker
            value={trigger.waveformId}
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
          className="justify-self-end ml-3 sm:ml-4"
        />
      </div>

      {selected && (
        <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
          <WaveformMini wf={selected} height={56} />
        </div>
      )}
    </div>
  );
}
