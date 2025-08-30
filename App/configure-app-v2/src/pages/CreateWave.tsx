import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { useSearchParams } from 'react-router-dom';

import { WaveformEditor } from '@/components/WaveformEditor';
import { DEFAULT_NEW_WAVEFORM } from '@/lib/defaultWaveforms';
import { useAppStore } from '@/lib/store';
import type { Waveform } from '@/lib/waveform';

// No selection by default: start with a localized default-new waveform

export default function CreateWave() {
  const { t } = useTranslation();
  const [searchParams, setSearchParams] = useSearchParams();
  const waveforms = useAppStore(s => s.waveforms);
  const addWaveform = useAppStore(s => s.addWaveform);
  const updateWaveform = useAppStore(s => s.updateWaveform);
  const removeWaveform = useAppStore(s => s.removeWaveform);

  const [selectedId, setSelectedId] = useState<string | ''>('');
  const [draft, setDraft] = useState<Waveform>(DEFAULT_NEW_WAVEFORM);

  const selected = useMemo(() => waveforms.find(w => w.id === selectedId), [waveforms, selectedId]);
  const selectedReadonly = !!selected?.readonly;

  function saveToLibrary() {
    if (selected) {
      updateWaveform(selected.id, draft);
    } else {
      const id = addWaveform(draft);
      setSelectedId(id);
    }
  }

  function newDraft() {
    setSelectedId('');
    setDraft(DEFAULT_NEW_WAVEFORM);
  }

  // If a ?select=ID param is present, select and load that waveform
  useEffect(() => {
    const id = searchParams.get('select');
    if (!id) return;
    const item = waveforms.find(w => w.id === id);
    if (item) {
      setSelectedId(item.id);
      setDraft({ name: item.name, totalTicks: item.totalTicks, changeAt: item.changeAt });
      // keep the param so reload preserves selection; or we could clear it:
      // setSearchParams(prev => { prev.delete('select'); return prev; }, { replace: true });
    }
  }, [searchParams, waveforms]);

  return (
    <div className="space-y-6">
      <div className="flex items-center gap-2">
        <h1 className="text-2xl font-semibold">{t('createWaveTitle')}</h1>
        <div className="ml-auto flex items-center gap-2">
          <select
            value={selectedId}
            onChange={e => {
              const id = e.target.value as string;
              setSelectedId(id);
              const item = waveforms.find(w => w.id === id);
              if (item) {
                setDraft({ name: item.name, totalTicks: item.totalTicks, changeAt: item.changeAt });
              } else {
                // No selection -> show default new waveform
                setDraft(DEFAULT_NEW_WAVEFORM);
              }
              // reflect selection in URL for deep link / navigation from other pages
              if (id) setSearchParams({ select: id });
              else setSearchParams({});
            }}
            className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
          >
            <option value="">{t('unsavedDraft')}</option>
            {waveforms.map(w => (
              <option key={w.id} value={w.id}>
                {w.name}
              </option>
            ))}
          </select>
          <button
            className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white"
            onClick={newDraft}
          >
            {t('newDraft')}
          </button>
          {selected && !selectedReadonly && (
            <button
              className="px-3 py-1.5 rounded bg-red-600 hover:bg-red-500 text-white"
              onClick={() => {
                removeWaveform(selected.id);
                newDraft();
              }}
            >
              {t('delete')}
            </button>
          )}
          <button
            className="px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900"
            onClick={saveToLibrary}
            disabled={selectedReadonly}
          >
            {selectedReadonly
              ? t('saveAs', { defaultValue: 'Save as new' })
              : selected
                ? t('save')
                : t('addToLibrary')}
          </button>
        </div>
      </div>

      <div className="flex items-center gap-2">
        <label className="text-sm">{t('name')}</label>
        <input
          value={draft.name}
          onChange={e => setDraft({ ...draft, name: e.target.value })}
          className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
        />
        <label className="text-sm ml-4">{t('totalTicks')}</label>
        <input
          type="number"
          min={2}
          value={draft.totalTicks}
          onChange={e => setDraft({ ...draft, totalTicks: Math.max(2, Number(e.target.value)) })}
          className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
        />
      </div>

      <WaveformEditor value={draft} onChange={setDraft} readOnly={selectedReadonly} />
    </div>
  );
}
