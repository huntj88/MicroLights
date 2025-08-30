import { useTranslation } from 'react-i18next';

import { WaveformMini } from '@/components/WaveformMini';
import type { Waveform } from '@/lib/waveform';

export type WaveformDoc = { id: string; readonly?: boolean } & Waveform;

export function WaveformPicker({
  value,
  onChange,
  waveforms,
  onCreate,
  onEdit,
  selectClassName = 'bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm',
}: {
  value?: string;
  onChange: (id: string | undefined) => void;
  waveforms: WaveformDoc[];
  onCreate: () => void;
  onEdit: (id: string) => void;
  previewHeight?: number;
  selectClassName?: string;
}) {
  const { t } = useTranslation();
  const selected = value ? (waveforms.find(w => w.id === value) ?? null) : null;

  const slug = (s: string) =>
    s
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/(^-|-$)/g, '');

  return (
    <div>
      <div className="flex items-center gap-2">
        <select
          value={value ?? ''}
          onChange={e => onChange(e.target.value || undefined)}
          className={selectClassName}
        >
          <option value="">{t('none')}</option>
          {waveforms.map(w => {
            const key = `waveformName.${slug(w.name)}`;
            const label = t(key, { defaultValue: w.name });
            return (
              <option key={w.id} value={w.id}>
                {label}
              </option>
            );
          })}
        </select>
        {selected && !selected.readonly && (
          <button
            className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
            onClick={() => onEdit(selected.id)}
            title={t('editWaveform')}
          >
            ✎
          </button>
        )}
        {selected && selected.readonly && (
          <span
            className="text-xs text-slate-400"
            title={t('readonly', { defaultValue: 'Read-only' })}
          >
            🔒
          </span>
        )}
        {
          <button
            className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
            onClick={onCreate}
            title={t('newWaveform')}
          >
            +
          </button>
        }
      </div>
      {selected && (
        <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
          <WaveformMini wf={selected} height={64} />
        </div>
      )}
    </div>
  );
}
