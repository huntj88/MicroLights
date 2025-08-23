import { useMemo } from 'react';
import { toast } from 'sonner';

import { useAppStore } from '@/lib/store';

import { ModeCard } from '../components/ModeCard';

export default function CreateSet() {
  const modes = useAppStore(s => s.modes);
  const connected = useAppStore(s => s.connected);
  const addMode = useAppStore(s => s.addMode);
  const connect = useAppStore(s => s.connect);
  const disconnect = useAppStore(s => s.disconnect);
  const send = useAppStore(s => s.send);

  const canAdd = useMemo(() => modes.length < 10, [modes.length]);

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold">Create / Set</h1>
        <div className="flex items-center gap-2">
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
