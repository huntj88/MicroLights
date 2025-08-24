import { useMemo, useState } from 'react';
import { toast } from 'sonner';

import { ModeCard } from '@/components/ModeCard';
import { useAppStore } from '@/lib/store';


export default function CreateMode() {
  const modes = useAppStore(s => s.modes);
  const connected = useAppStore(s => s.connected);
  const modeSets = useAppStore(s => s.modeSets);
  const addMode = useAppStore(s => s.addMode);
  const connect = useAppStore(s => s.connect);
  const disconnect = useAppStore(s => s.disconnect);
  const send = useAppStore(s => s.send);

  // store actions for mode set library workflow
  const newModeSetDraft = useAppStore(s => s.newModeSetDraft);
  const saveCurrentModeSet = useAppStore(s => s.saveCurrentModeSet);
  const updateModeSet = useAppStore(s => s.updateModeSet);
  const loadModeSet = useAppStore(s => s.loadModeSet);
  const removeModeSet = useAppStore(s => s.removeModeSet);

  const [selectedSetId, setSelectedSetId] = useState<string>('');
  const [draftName, setDraftName] = useState<string>('');

  const canAdd = useMemo(() => modes.length < 10, [modes.length]);
  const selectedSet = useMemo(() => modeSets.find(s => s.id === selectedSetId), [modeSets, selectedSetId]);

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold">Create / Mode</h1>
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-2 mr-4">
            {/* Name input moved below header to align with Waveform page */}
            <select
              value={selectedSetId}
              onChange={e => {
                const id = e.target.value;
                setSelectedSetId(id);
                if (!id) return;
                loadModeSet(id);
                setDraftName(modeSets.find(s => s.id === id)?.name ?? '');
                toast.success('Mode loaded');
              }}
              className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
            >
              <option value="">(Unsaved Draft)</option>
              {modeSets.map(s => (
                <option key={s.id} value={s.id}>{s.name}</option>
              ))}
            </select>
            <button
              type="button"
              onClick={() => {
                newModeSetDraft();
                setSelectedSetId('');
                setDraftName('New Mode');
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
                  if (confirm('Delete this mode?')) {
                    removeModeSet(selectedSetId);
                    setSelectedSetId('');
                    newModeSetDraft();
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
                  updateModeSet(selectedSet.id, draftName.trim() || selectedSet.name);
                  toast.success('Mode saved');
                } else {
                  const id = saveCurrentModeSet(draftName);
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
        <div className="text-xs text-slate-400">Tip: Save the current configuration as a Mode, or load an existing Mode to edit and save changes.</div>
      </div>

      <div className="flex items-center gap-2">
        <div className="text-xs text-slate-400">
          Tip: {modes.length > 1
            ? 'Finger-specific options are shown below for each Mode Card.'
            : 'Add another Mode Card to reveal finger-specific options.'}
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-4">
        {modes.map(m => (
          <ModeCard key={m.id} mode={m} showFingerOptions={modes.length > 1} />
        ))}
        <button
          type="button"
          disabled={!canAdd}
          onClick={() => addMode()}
          className={`rounded-xl border border-slate-700/50 bg-bg-card p-4 flex items-center justify-center text-sm ${canAdd ? 'hover:bg-slate-700/40 text-white' : 'opacity-50 cursor-not-allowed text-slate-400'}`}
          title={canAdd ? 'Add a new Mode' : 'Maximum number of Modes reached'}
        >
          + Add Mode Card
        </button>
      </div>
    </div>
  );
}
