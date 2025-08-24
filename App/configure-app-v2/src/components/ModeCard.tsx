import { clsx } from 'clsx';
import { HexColorPicker } from 'react-colorful';

import { FINGERS_BY_HAND, type Finger, type Hand } from '@/lib/fingers';
import { useAppStore, type Mode } from '@/lib/store';
import { toSegments, type Waveform } from '@/lib/waveform';

export function ModeCard({ mode, showFingerOptions = true }: { mode: Mode; showFingerOptions?: boolean }) {
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

  // accelerometer actions
  const addAccelTrigger = useAppStore(s => s.addAccelTrigger);
  const removeAccelTrigger = useAppStore(s => s.removeAccelTrigger);
  const setAccelTriggerThreshold = useAppStore(s => s.setAccelTriggerThreshold);
  const setAccelTriggerWaveform = useAppStore(s => s.setAccelTriggerWaveform);

  const selectedWaveform = waveforms.find(w => w.id === mode.waveformId) ?? null;

  return (
    <div className="rounded-xl border border-slate-700/50 bg-bg-card p-4">
      <div className="grid grid-cols-1 md:grid-cols-[1fr_auto] gap-4">
        <div className="space-y-3">
          {showFingerOptions && (
            <div>
              <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">Fingers</div>
              <div className="flex gap-2 mb-2">
                <button
                  className="px-2 py-1 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-xs"
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
          )}

          {showFingerOptions && <div className="h-px bg-slate-700/40 my-3" />}

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
                  className="px-2 py-1 rounded border border-red-600/40 text-red-400 hover:bg-red-600/10 text-xs"
                  onClick={() => removeWaveform(mode.waveformId!)}
                  title="Delete selected waveform"
                >
                  Remove
                </button>
              )}
            </div>
            {selectedWaveform && (
              <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
                <WaveformPreview wf={selectedWaveform} />
              </div>
            )}
          </div>

          {/* Accelerometer section */}
          <div className="mt-3">
            <div className="flex items-center justify-between">
              <div className="text-xs uppercase tracking-wide text-slate-400">Accelerometer</div>
            </div>

            {(mode.accel?.triggers?.length ?? 0) > 0 && (
              <div className="mt-2 space-y-2">
                {(mode.accel?.triggers ?? []).map((t, i) => {
                  const accelWf = t.waveformId ? waveforms.find(w => w.id === t.waveformId) ?? null : null;
                  const ALLOWED = [2, 4, 8, 12, 16];
                  const prevThresh = i > 0 ? mode.accel?.triggers?.[i - 1]?.threshold : undefined;
                  const allowedAfterPrev = prevThresh == null ? ALLOWED : ALLOWED.filter(v => v > prevThresh);
                  return (
                    <div key={i} className="grid grid-cols-[auto_1fr_auto] items-center gap-2">
                      <div className="text-xs text-slate-400">Threshold</div>
                      <select
                        value={t.threshold}
                        onChange={e => setAccelTriggerThreshold(mode.id, i, Number(e.target.value))}
                        className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
                      >
                        {allowedAfterPrev.map(v => (
                          <option key={v} value={v}>{v}</option>
                        ))}
                      </select>
                      <button
                        className="px-2 py-1 rounded border border-red-600/40 text-red-400 hover:bg-red-600/10 text-xs"
                        onClick={() => removeAccelTrigger(mode.id, i)}
                      >
                        Remove
                      </button>

                      <div className="text-xs text-slate-400">Waveform</div>
                      <select
                        value={t.waveformId ?? ''}
                        onChange={e => setAccelTriggerWaveform(mode.id, i, e.target.value || undefined)}
                        className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
                      >
                        <option value="">None</option>
                        {waveforms.map(w => (
                          <option key={w.id} value={w.id}>
                            {w.name}
                          </option>
                        ))}
                      </select>
                      <div />

                      {accelWf && (
                        <div className="col-span-3">
                          <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
                            <WaveformPreview wf={accelWf} />
                          </div>
                        </div>
                      )}
                    </div>
                  );
                })}
              </div>
            )}

            {(mode.accel?.triggers?.length ?? 0) < 2 && (
              <div className="mt-2">
                <button
                  className="px-2 py-1 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-xs"
                  onClick={() => addAccelTrigger(mode.id)}
                >
                  + Add Trigger
                </button>
              </div>
            )}
          </div>
        </div>

        <div className="space-y-3 justify-self-start md:pl-2">
          <div>
            <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">Case Light Color</div>
            <div>
              <HexColorPicker
                color={mode.color}
                onChange={hex => setColor(mode.id, hex)}
                style={{ width: 120, height: 120 }}
              />
            </div>
          </div>
        </div>
      </div>

      <div className="flex justify-end mt-4">
        <button
          className="px-2 py-1 rounded border border-red-600/40 text-red-400 hover:bg-red-600/10 text-xs"
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

function WaveformPreview({ wf }: { wf: Waveform }) {
  const segs = toSegments(wf);
  const height = 40;
  const pad = 0; // no left padding so the preview aligns flush
  const totalWidth = pad + wf.totalTicks; // no right pad to maximize width
  const yFor = (out: 'high' | 'low') => (out === 'high' ? 8 : height - 8);
  return (
    <svg
      className="w-full h-16"
      viewBox={`0 0 ${totalWidth} ${height}`}
      preserveAspectRatio="none"
      role="img"
      aria-label="Waveform preview"
    >
      {/* center dashed line */}
      <line x1={pad} y1={height / 2} x2={totalWidth} y2={height / 2} stroke="#334155" strokeDasharray="4 4" />

      {/* step lines: horizontal thick, vertical thin */}
      <g stroke="#22c55e" fill="none" strokeLinecap="butt">
        {/* horizontals */}
        {segs.map((s, i) => (
          <line key={`h-${i}`} x1={pad + s.from} y1={yFor(s.output)} x2={pad + s.to} y2={yFor(s.output)} strokeWidth={2} />
        ))}
        {/* verticals */}
        {segs.slice(0, -1).map((s, i) => {
          const next = segs[i + 1];
          const x = pad + s.to;
          return <line key={`v-${i}`} x1={x} y1={yFor(s.output)} x2={x} y2={yFor(next.output)} strokeWidth={1} />;
        })}
      </g>
    </svg>
  );
}
