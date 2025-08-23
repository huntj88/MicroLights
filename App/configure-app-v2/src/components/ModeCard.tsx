import { clsx } from 'clsx';
import { HexColorPicker } from 'react-colorful';

import { FINGERS_BY_HAND, type Finger, type Hand } from '@/lib/fingers';
import { useAppStore, type Mode } from '@/lib/store';

export function ModeCard({ mode }: { mode: Mode }) {
  const owner = useAppStore(s => s.fingerOwner);
  const setColor = useAppStore(s => s.setColor);
  const selectAll = useAppStore(s => s.selectAll);
  const assign = useAppStore(s => s.assignFinger);
  const unassign = useAppStore(s => s.unassignFinger);
  const remove = useAppStore(s => s.removeMode);

  const waveforms = useAppStore(s => s.waveforms);
  const setWaveform = useAppStore(s => s.setWaveform);
  const addWaveform = useAppStore(s => s.addWaveform);
  const removeWaveform = useAppStore(s => s.removeWaveform);

  return (
    <div className="rounded-xl border border-slate-700/50 bg-bg-card p-4">
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
            </div>
            {/* Two-column hand layout */}
            <div className="grid grid-cols-2 gap-4">
              <div>
                <div className="text-xs text-slate-400 mb-1">Left</div>
                <div className="flex flex-col gap-2">
                  {FINGERS_BY_HAND.L.map(f => (
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
                <div className="text-xs text-slate-400 mb-1">Right</div>
                <div className="flex flex-col gap-2">
                  {FINGERS_BY_HAND.R.map(f => (
                    <FingerChip
                      key={f}
                      finger={f}
                      owned={owner[f] === mode.id}
                      onToggle={() => (owner[f] === mode.id ? unassign(mode.id, f) : assign(mode.id, f))}
                    />
                  ))}
                </div>
              </div>
            </div>
          </div>

          <div>
            <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">Waveform</div>
            <div className="flex items-center gap-2">
              <select
                value={mode.waveformId ?? ''}
                onChange={e => setWaveform(mode.id, e.target.value || undefined)}
                className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm flex-1"
              >
                <option value="">None</option>
                {waveforms.map(w => (
                  <option key={w.id} value={w.id}>
                    {w.name}
                  </option>
                ))}
              </select>
              <button
                className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
                onClick={() => {
                  const id = addWaveform({ name: 'New Wave', totalTicks: 16, changeAt: [{ tick: 0, output: 'high' }] });
                  setWaveform(mode.id, id);
                }}
                title="Create new waveform"
              >
                +
              </button>
              {mode.waveformId && (
                <button
                  className="px-2 py-1 rounded bg-red-600 hover:bg-red-500 text-white text-xs"
                  onClick={() => removeWaveform(mode.waveformId!)}
                  title="Delete selected waveform"
                >
                  Remove
                </button>
              )}
            </div>
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
