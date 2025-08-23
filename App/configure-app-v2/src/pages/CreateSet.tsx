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
  const saveCurrentSet = useAppStore(s => s.saveCurrentSet);
  const loadSet = useAppStore(s => s.loadSet);
  const renameSet = useAppStore(s => s.renameSet);
  const removeSet = useAppStore(s => s.removeSet);

  const [setName, setSetName] = useState('');
  const [selectedSetId, setSelectedSetId] = useState<string>('');

  const canAdd = useMemo(() => modes.length < 10, [modes.length]);

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold">Create / Set</h1>
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-2 mr-4">
            <input
              value={setName}
              onChange={e => setSetName(e.target.value)}
              placeholder="Set name"
              className="w-40 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
            />
            <button
              type="button"
              onClick={() => {
                const id = saveCurrentSet(setName);
                setSelectedSetId(id);
                toast.success('Set saved');
              }}
              className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white"
            >
              Save Set
            </button>
            <select
              value={selectedSetId}
              onChange={e => setSelectedSetId(e.target.value)}
              className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
            >
              <option value="">(Select Set)</option>
              {sets.map(s => (
                <option key={s.id} value={s.id}>{s.name}</option>
              ))}
            </select>
            <button
              type="button"
              disabled={!selectedSetId}
              onClick={() => {
                if (!selectedSetId) return;
                loadSet(selectedSetId);
                toast.success('Set loaded');
              }}
              className={`px-3 py-1.5 rounded ${selectedSetId ? 'bg-slate-700 hover:bg-slate-600 text-white' : 'bg-slate-700/40 text-slate-400 cursor-not-allowed'}`}
            >
              Load
            </button>
            <button
              type="button"
              disabled={!selectedSetId}
              onClick={() => {
                if (!selectedSetId) return;
                const name = prompt('Rename set to:', sets.find(s => s.id === selectedSetId)?.name || '')?.trim();
                if (name) renameSet(selectedSetId, name);
              }}
              className={`px-3 py-1.5 rounded ${selectedSetId ? 'bg-slate-700 hover:bg-slate-600 text-white' : 'bg-slate-700/40 text-slate-400 cursor-not-allowed'}`}
            >
              Rename
            </button>
            <button
              type="button"
              disabled={!selectedSetId}
              onClick={() => {
                if (!selectedSetId) return;
                if (confirm('Delete this set?')) {
                  removeSet(selectedSetId);
                  setSelectedSetId('');
                }
              }}
              className={`px-3 py-1.5 rounded ${selectedSetId ? 'bg-red-600 hover:bg-red-500 text-white' : 'bg-slate-700/40 text-slate-400 cursor-not-allowed'}`}
            >
              Delete
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

      <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-4">
        {modes.map(m => (
          <ModeCard key={m.id} mode={m} />
        ))}
      </div>
    </div>
  );
}
