import { useTranslation } from 'react-i18next';

import { CaseLightColorTogglePicker } from '@/components/CaseLightColorTogglePicker';
import { WaveformMini } from '@/components/WaveformMini';
import { WaveformPicker } from '@/components/WaveformPicker';
import type { Waveform } from '@/lib/waveform';

type Trigger = {
  threshold: number;
  color?: string;
  waveformId?: string;
};

type WaveformDoc = { id: string } & Waveform;

const ALLOWED = [2, 4, 8, 12, 16] as const;

export function AccelTriggerRow({
  trigger,
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
  previewBelow = true,
}: {
  trigger: Trigger;
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
  disabledColor?: string;
  previewBelow?: boolean;
}) {
  const { t } = useTranslation();
  const allowedAfterPrev =
    prevThreshold == null ? [...ALLOWED] : (ALLOWED.filter(v => v > prevThreshold) as number[]);

  const selected = trigger.waveformId
    ? (waveforms.find(w => w.id === trigger.waveformId) ?? null)
    : null;

  const effectiveColor = trigger.color ?? defaultColor;
  // color enabled state is derived in ColorTogglePicker

  return (
    <div>
      <div className="grid grid-cols-[auto_1fr_auto] items-center gap-2">
        <div className="text-xs text-slate-400">{t('thresholdXg')}</div>
        <select
          value={trigger.threshold}
          onChange={e => onChangeThreshold(Number(e.target.value))}
          className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
          aria-label={t('accelerationThresholdAria')}
        >
          {allowedAfterPrev.map(v => (
            <option key={v} value={v}>{`${v} g`}</option>
          ))}
        </select>
        <button
          className="px-2 py-1 rounded border border-red-600/40 text-red-400 hover:bg-red-600/10 text-xs"
          onClick={onRemove}
        >
          {t('remove')}
        </button>

        <div className="text-xs text-slate-400">{t('waveform')}</div>
        <div className="flex items-center gap-2">
          <WaveformPicker
            value={trigger.waveformId}
            onChange={onChangeWaveform}
            waveforms={waveforms}
            onCreate={onCreateWaveform}
            onEdit={onEditWaveform}
            showPreview={false}
          />
        </div>

        <CaseLightColorTogglePicker
          color={effectiveColor}
          onToggle={onToggleColor}
          onChange={onColorChange}
          className="justify-self-end ml-3 sm:ml-4"
        />
      </div>

      {previewBelow && selected && (
        <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
          <WaveformMini wf={selected} height={56} />
        </div>
      )}
    </div>
  );
}
