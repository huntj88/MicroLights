import { clsx } from 'clsx';
import { HexColorPicker } from 'react-colorful';

import { ALL_FINGERS, type Finger, type Hand } from '@/lib/fingers';
import { useAppStore, type Mode } from '@/lib/store';

export function ModeCard({ mode }: { mode: Mode }) {
  const owner = useAppStore(s => s.fingerOwner);
  const rename = useAppStore(s => s.renameMode);
  const toggle = useAppStore(s => s.toggleMode);
  const setColor = useAppStore(s => s.setColor);
  const selectAll = useAppStore(s => s.selectAll);
  const selectHand = useAppStore(s => s.selectHand);
  const assign = useAppStore(s => s.assignFinger);
  const unassign = useAppStore(s => s.unassignFinger);
  const remove = useAppStore(s => s.removeMode);

  return (
    <div className="rounded-xl border border-slate-700/50 bg-bg-card p-4">
      <div className="flex items-center gap-2 mb-3">
        <input
          value={mode.name}
          onChange={e => rename(mode.id, e.target.value)}
          className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm flex-1"
        />
        <label className="flex items-center gap-2 text-sm">
          <input
            type="checkbox"
            checked={mode.enabled}
            onChange={e => toggle(mode.id, e.target.checked)}
          />
          Enabled
        </label>
      </div>

      <div className="grid grid-cols-2 gap-4">
        <div className="space-y-3">
          <div>
            <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">Fingers</div>
            <div className="flex gap-2 mb-2">
              <button
                className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
                onClick={() => selectAll(mode.id)}
              >
                All
              </button>
              <button
                className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
                onClick={() => selectHand(mode.id, 'L')}
              >
                Left
              </button>
              <button
                className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
                onClick={() => selectHand(mode.id, 'R')}
              >
                Right
              </button>
            </div>
            <div className="grid grid-cols-2 gap-2">
              {ALL_FINGERS.map(f => (
                <FingerChip
                  key={f}
                  finger={f}
                  owned={owner[f] === mode.id}
                  onToggle={() => (owner[f] === mode.id ? unassign(mode.id, f) : assign(mode.id, f))}
                />
              ))}
            </div>
          </div>

          <div>
            <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">Waveform</div>
            <select
              value={mode.waveformId ?? ''}
              onChange={e => {
                const id = e.target.value || undefined;
                useAppStore.getState().setWaveform(mode.id, id);
              }}
              className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
            >
              <option value="">None</option>
              <option value="pulse">Pulse</option>
              <option value="wave">Wave</option>
              <option value="sparkle">Sparkle</option>
            </select>
          </div>
        </div>

        <div className="space-y-3">
          <div>
            <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">Color</div>
            <HexColorPicker color={mode.color} onChange={hex => setColor(mode.id, hex)} />
          </div>
        </div>
      </div>

      <div className="flex justify-end mt-4">
        <button
          className="px-2 py-1 rounded bg-red-600 hover:bg-red-500 text-white text-xs"
          onClick={() => remove(mode.id)}
        >
          Remove
        </button>
      </div>
    </div>
  );
}

function FingerChip({ finger, owned, onToggle }: { finger: Finger; owned: boolean; onToggle: () => void }) {
  const [hand, name] = finger.split('-') as [Hand, string];
  return (
    <button
      className={clsx(
        'px-2 py-1 rounded text-xs border border-slate-700/50',
        owned ? 'bg-fg-ring/80 text-slate-900' : 'bg-slate-800 text-slate-200 hover:bg-slate-700'
      )}
      onClick={onToggle}
    >
      <span className="opacity-70 mr-1">{hand === 'L' ? 'L' : 'R'}</span>
      {name}
    </button>
  );
}
