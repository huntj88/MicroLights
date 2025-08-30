import { WaveformMini } from '@/components/WaveformMini';
import type { Waveform } from '@/lib/waveform';

export type WaveformDoc = { id: string } & Waveform;

export function WaveformPicker({
  value,
  onChange,
  waveforms,
  onCreate,
  onEdit,
  showPreview = true,
  previewHeight = 64,
  selectClassName = 'bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm',
}: {
  value?: string;
  onChange: (id: string | undefined) => void;
  waveforms: WaveformDoc[];
  onCreate: () => void;
  onEdit: (id: string) => void;
  showPreview?: boolean;
  previewHeight?: number;
  selectClassName?: string;
}) {
  const selected = value ? waveforms.find(w => w.id === value) ?? null : null;

  return (
    <div>
      <div className="flex items-center gap-2">
        <select
          value={value ?? ''}
          onChange={e => onChange(e.target.value || undefined)}
          className={selectClassName}
        >
          <option value="">None</option>
          {waveforms.map(w => (
            <option key={w.id} value={w.id}>
              {w.name}
            </option>
          ))}
        </select>
        {!selected && (
          <button
            className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
            onClick={onCreate}
            title="Create new waveform"
          >
            +
          </button>
        )}
        {selected && (
          <button
            className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
            onClick={() => onEdit(selected.id)}
            title="Edit selected waveform"
          >
            ✎
          </button>
        )}
      </div>
      {showPreview && selected && (
        <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
          <WaveformMini wf={selected} height={previewHeight} />
        </div>
      )}
    </div>
  );
}
