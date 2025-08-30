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
  disabledColor = '#000000',
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
  const allowedAfterPrev =
    prevThreshold == null ? [...ALLOWED] : (ALLOWED.filter(v => v > prevThreshold) as number[]);

  const selected = trigger.waveformId
    ? (waveforms.find(w => w.id === trigger.waveformId) ?? null)
    : null;

  const effectiveColor = trigger.color ?? defaultColor;
  const enabled = effectiveColor !== disabledColor;

  return (
    <div>
      <div className="grid grid-cols-[auto_1fr_auto] items-center gap-2">
        <div className="text-xs text-slate-400">Threshold (× g)</div>
        <select
          value={trigger.threshold}
          onChange={e => onChangeThreshold(Number(e.target.value))}
          className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
          aria-label="Acceleration threshold in multiples of g"
        >
          {allowedAfterPrev.map(v => (
            <option key={v} value={v}>{`${v} g`}</option>
          ))}
        </select>
        <button
          className="px-2 py-1 rounded border border-red-600/40 text-red-400 hover:bg-red-600/10 text-xs"
          onClick={onRemove}
        >
          Remove
        </button>

        <div className="text-xs text-slate-400">Waveform</div>
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

        <div className="flex items-center gap-2 justify-self-end ml-3 sm:ml-4">
          <input
            type="checkbox"
            aria-label="Enable trigger color"
            title="Enable trigger color"
            checked={enabled}
            onChange={e => onToggleColor(e.target.checked)}
            className="accent-fg-ring"
          />
          <input
            type="color"
            value={effectiveColor}
            onChange={e => onColorChange(e.target.value)}
            disabled={!enabled}
            className="h-8 w-12 p-0 bg-transparent border border-slate-700/50 rounded disabled:opacity-50"
            aria-label="Trigger color"
            title={enabled ? 'Trigger color' : 'Enable trigger color to choose color'}
          />
        </div>
      </div>

      {previewBelow && selected && (
        <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
          <WaveformMini wf={selected} height={56} />
        </div>
      )}
    </div>
  );
}
