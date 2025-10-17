import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { useSearchParams } from 'react-router-dom';

import { BulbModeWaveformEditor } from '@/components/BulbModeWaveformEditor';
import type { BulbModeWaveform } from '@/lib/bulbModeWaveform';
import { DEFAULT_NEW_BULB_MODE_WAVEFORM } from '@/lib/defaultWaveforms';
import { useAppStore } from '@/lib/store';

// No selection by default: start with a localized default-new waveform

export default function CreateWave() {
  const { t } = useTranslation();
  const [searchParams, setSearchParams] = useSearchParams();
  const bulbModeWaveforms = useAppStore(s => s.bulbModeWaveforms);
  const lastSelectedWaveformId = useAppStore(s => s.lastSelectedBulbModeWaveformId);
  const setLastSelectedWaveformId = useAppStore(s => s.setLastSelectedBulbModeWaveformId);
  const addWaveform = useAppStore(s => s.addBulbModeWaveform);
  const updateWaveform = useAppStore(s => s.updateBulbModeWaveform);
  const removeWaveform = useAppStore(s => s.removeBulbModeWaveform);

  const [selectedId, setSelectedId] = useState<string | ''>('');
  const [draft, setDraft] = useState<BulbModeWaveform>(DEFAULT_NEW_BULB_MODE_WAVEFORM);

  const selected = useMemo(
    () => bulbModeWaveforms.find(w => w.id === selectedId),
    [bulbModeWaveforms, selectedId],
  );
  const selectedReadonly = !!selected?.readonly;

  function saveToLibrary() {
    if (selected) {
      updateWaveform(selected.id, draft);
      setLastSelectedWaveformId(selected.id);
    } else {
      const id = addWaveform(draft);
      setSelectedId(id);
      setLastSelectedWaveformId(id);
    }
  }

  function newDraft() {
    setSelectedId('');
    setDraft(DEFAULT_NEW_BULB_MODE_WAVEFORM);
    setLastSelectedWaveformId(null);
  }

  // If a ?select=ID param is present, select and load that waveform
  useEffect(() => {
    const id = searchParams.get('select');
    if (!id) return;
    const item = bulbModeWaveforms.find(w => w.id === id);
    if (item) {
      setSelectedId(item.id);
      setDraft({ name: item.name, totalTicks: item.totalTicks, changeAt: item.changeAt });
      setLastSelectedWaveformId(item.id);
      // keep the param so reload preserves selection; or we could clear it:
      // setSearchParams(prev => { prev.delete('select'); return prev; }, { replace: true });
    }
  }, [searchParams, bulbModeWaveforms, setLastSelectedWaveformId]);

  // Default to the most recently opened waveform from store if no URL param drives selection
  useEffect(() => {
    if (searchParams.get('select')) return;
    if (selectedId) return;
    if (!lastSelectedWaveformId) return;
    const item = bulbModeWaveforms.find(w => w.id === lastSelectedWaveformId);
    if (!item) return;
    setSelectedId(item.id);
    setDraft({ name: item.name, totalTicks: item.totalTicks, changeAt: item.changeAt });
  }, [searchParams, selectedId, lastSelectedWaveformId, bulbModeWaveforms]);

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
              setLastSelectedWaveformId(id || null);
              const item = bulbModeWaveforms.find(w => w.id === id);
              if (item) {
                setDraft({ name: item.name, totalTicks: item.totalTicks, changeAt: item.changeAt });
              } else {
                // No selection -> show default new waveform
                setDraft(DEFAULT_NEW_BULB_MODE_WAVEFORM);
              }
              // reflect selection in URL for deep link / navigation from other pages
              if (id) setSearchParams({ select: id });
              else setSearchParams({});
            }}
            className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
          >
            <option value="">{t('unsavedDraft')}</option>
            {bulbModeWaveforms.map(w => (
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
            {selectedReadonly ? t('duplicate') : selected ? t('save') : t('addToLibrary')}
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

      <BulbModeWaveformEditor value={draft} onChange={setDraft} readOnly={selectedReadonly} />
    </div>
  );
}
