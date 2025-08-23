import { useMemo, useState } from 'react';
import { toast } from 'sonner';

import { useAppStore } from '@/lib/store';

import { ModeCard } from '../components/ModeCard';

export default function CreateSet() {
  const modes = useAppStore(s => s.modes);
  const connected = useAppStore(s => s.connected);
  const sets = useAppStore(s => s.sets);
  const addMode = useAppStore(s => s.addMode);
  const connect = useAppStore(s => s.connect);
  const disconnect = useAppStore(s => s.disconnect);
  const send = useAppStore(s => s.send);

  // new store actions for set workflow
  const newSetDraft = useAppStore(s => s.newSetDraft);
  const saveCurrentSet = useAppStore(s => s.saveCurrentSet);
  const updateSet = useAppStore(s => s.updateSet);
  const loadSet = useAppStore(s => s.loadSet);
  const removeSet = useAppStore(s => s.removeSet);

  const [selectedSetId, setSelectedSetId] = useState<string>('');
  const [draftName, setDraftName] = useState<string>('');

  const canAdd = useMemo(() => modes.length < 10, [modes.length]);
  const selectedSet = useMemo(() => sets.find(s => s.id === selectedSetId), [sets, selectedSetId]);

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold">Create / Set</h1>
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-2 mr-4">
            {/* Name input moved below header to align with Waveform page */}
            <select
              value={selectedSetId}
              onChange={e => {
                const id = e.target.value;
                setSelectedSetId(id);
                if (!id) return;
                loadSet(id);
                setDraftName(sets.find(s => s.id === id)?.name ?? '');
                toast.success('Set loaded');
              }}
              className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
            >
              <option value="">(Unsaved Draft)</option>
              {sets.map(s => (
                <option key={s.id} value={s.id}>{s.name}</option>
              ))}
            </select>
            <button
              type="button"
              onClick={() => {
                newSetDraft();
                setSelectedSetId('');
                setDraftName('New Set');
              }}
              className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white"
            >
              New Draft
            </button>
            {selectedSet && (
              <button
                type="button"
                onClick={() => {
                  if (!selectedSetId) return;
                  if (confirm('Delete this set?')) {
                    removeSet(selectedSetId);
                    setSelectedSetId('');
                    newSetDraft();
                    setDraftName('');
                  }
                }}
                className="px-3 py-1.5 rounded bg-red-600 hover:bg-red-500 text-white"
              >
                Delete
              </button>
            )}
            <button
              type="button"
              onClick={() => {
                if (selectedSet) {
                  updateSet(selectedSet.id, draftName.trim() || selectedSet.name);
                  toast.success('Set saved');
                } else {
                  const id = saveCurrentSet(draftName);
                  setSelectedSetId(id);
                  toast.success('Added to Library');
                }
              }}
              className="px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900"
            >
              {selectedSet ? 'Save' : 'Add to Library'}
            </button>
          </div>

          <button
            type="button"
            onClick={() => (connected ? disconnect() : connect())}
            className={`px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white`}
          >
            {connected ? 'Disconnect' : 'Connect'}
          </button>
          <button
            type="button"
            onClick={async () => {
              await send();
              toast.success('Program sent');
            }}
            className="px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900"
          >
            Send
          </button>
          <button
            type="button"
            disabled={!canAdd}
            onClick={() => addMode()}
            className={`px-3 py-1.5 rounded ${canAdd ? 'bg-slate-700 hover:bg-slate-600 text-white' : 'bg-slate-700/40 text-slate-400 cursor-not-allowed'}`}
          >
            Add Mode
          </button>
        </div>
      </div>

      {/* Name row aligned like Waveform page */}
      <div className="flex items-center gap-2">
        <label className="text-sm">Name</label>
        <input
          value={draftName}
          onChange={e => setDraftName(e.target.value)}
          className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
        />
      </div>

      <div className="flex items-center gap-2">
        <div className="text-xs text-slate-400">Tip: Save the current configuration as a Set, or load an existing Set to edit and save changes.</div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-4">
        {modes.map(m => (
          <ModeCard key={m.id} mode={m} />
        ))}
      </div>
    </div>
  );
}
