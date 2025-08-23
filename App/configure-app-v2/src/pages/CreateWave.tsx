import { useMemo, useState } from 'react';

import { WaveformEditor } from '@/components/WaveformEditor';
import { useAppStore } from '@/lib/store';
import type { Waveform } from '@/lib/waveform';

const initial: Waveform = {
  name: 'example',
  totalTicks: 33,
  changeAt: [
    { tick: 0, output: 'high' },
    { tick: 11, output: 'low' },
    { tick: 12, output: 'high' },
    { tick: 22, output: 'low' },
  ],
};

export default function CreateWave() {
  const waveforms = useAppStore(s => s.waveforms);
  const addWaveform = useAppStore(s => s.addWaveform);
  const updateWaveform = useAppStore(s => s.updateWaveform);
  const removeWaveform = useAppStore(s => s.removeWaveform);

  const [selectedId, setSelectedId] = useState<string | ''>('');
  const [draft, setDraft] = useState<Waveform>(initial);

  const selected = useMemo(() => waveforms.find(w => w.id === selectedId), [waveforms, selectedId]);

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
    setDraft({ name: 'New Wave', totalTicks: 16, changeAt: [{ tick: 0, output: 'high' }] });
  }

  return (
    <div className="space-y-6">
      <div className="flex items-center gap-2">
        <h1 className="text-2xl font-semibold">Create / Wave</h1>
        <div className="ml-auto flex items-center gap-2">
          <select
            value={selectedId}
            onChange={e => {
              const id = e.target.value as string;
              setSelectedId(id);
              const item = waveforms.find(w => w.id === id);
              if (item) setDraft({ name: item.name, totalTicks: item.totalTicks, changeAt: item.changeAt });
            }}
            className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
          >
            <option value="">(Unsaved Draft)</option>
            {waveforms.map(w => (
              <option key={w.id} value={w.id}>
                {w.name}
              </option>
            ))}
          </select>
          <button className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white" onClick={newDraft}>
            New Draft
          </button>
          {selected && (
            <button
              className="px-3 py-1.5 rounded bg-red-600 hover:bg-red-500 text-white"
              onClick={() => {
                removeWaveform(selected.id);
                newDraft();
              }}
            >
              Delete
            </button>
          )}
          <button className="px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900" onClick={saveToLibrary}>
            {selected ? 'Save' : 'Add to Library'}
          </button>
        </div>
      </div>

      <div className="flex items-center gap-2">
        <label className="text-sm">Name</label>
        <input
          value={draft.name}
          onChange={e => setDraft({ ...draft, name: e.target.value })}
          className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
        />
        <label className="text-sm ml-4">Total Ticks</label>
        <input
          type="number"
          min={2}
          value={draft.totalTicks}
          onChange={e => setDraft({ ...draft, totalTicks: Math.max(2, Number(e.target.value)) })}
          className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
        />
      </div>

      <WaveformEditor value={draft} onChange={setDraft} />
    </div>
  );
}
