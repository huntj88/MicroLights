import { clsx } from 'clsx';
import { useMemo, useState } from 'react';
import { useNavigate } from 'react-router-dom';

import { Modal } from '@/components/Modal';
import { WaveformEditor } from '@/components/WaveformEditor';
import { WaveformMini } from '@/components/WaveformMini';
import { FINGERS_BY_HAND, type Finger, type Hand } from '@/lib/fingers';
import { useAppStore, type Mode } from '@/lib/store';
import type { Waveform } from '@/lib/waveform';

export function ModeCard({ mode, showFingerOptions = true }: { mode: Mode; showFingerOptions?: boolean }) {
  const navigate = useNavigate();
  const owner = useAppStore(s => s.fingerOwner);
  const setColor = useAppStore(s => s.setColor);
  const selectAll = useAppStore(s => s.selectAll);
  const assign = useAppStore(s => s.assignFinger);
  const unassign = useAppStore(s => s.unassignFinger);
  const remove = useAppStore(s => s.removeMode);

  const waveforms = useAppStore(s => s.waveforms);
  const setWaveform = useAppStore(s => s.setWaveform);
  const addWaveform = useAppStore(s => s.addWaveform);

  // accelerometer actions
  const addAccelTrigger = useAppStore(s => s.addAccelTrigger);
  const removeAccelTrigger = useAppStore(s => s.removeAccelTrigger);
  const setAccelTriggerThreshold = useAppStore(s => s.setAccelTriggerThreshold);
  const setAccelTriggerWaveform = useAppStore(s => s.setAccelTriggerWaveform);
  const setAccelTriggerColor = useAppStore(s => s.setAccelTriggerColor);

  const exportModeAsJSON = useAppStore(s => s.exportModeAsJSON);

  const selectedWaveform = waveforms.find(w => w.id === mode.waveformId) ?? null;

  // Type-safe triggers list
  type Trigger = NonNullable<Mode['accel']>['triggers'][number];
  const triggers: Trigger[] = mode.accel?.triggers ?? [];

  // Popup state for creating a new waveform inline
  const [wfModalOpen, setWfModalOpen] = useState(false);
  const [wfDraft, setWfDraft] = useState<Waveform>({ name: 'New Wave', totalTicks: 16, changeAt: [{ tick: 0, output: 'high' }] });
  const canSaveDraft = useMemo(() => wfDraft.name.trim().length > 0 && wfDraft.totalTicks >= 2 && wfDraft.changeAt.length > 0 && wfDraft.changeAt[0]?.tick === 0, [wfDraft]);

  return (
    <div className="rounded-xl border border-slate-700/50 bg-bg-card p-4">
      <div className="grid grid-cols-1 gap-4">
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
                  setWfDraft({ name: 'New Wave', totalTicks: 16, changeAt: [{ tick: 0, output: 'high' }] });
                  setWfModalOpen(true);
                }}
                title="Create new waveform"
              >
                +
              </button>
              {mode.waveformId && (
                <button
                  className="px-2 py-1 rounded border border-red-600/40 text-red-400 hover:bg-red-600/10 text-xs"
                  onClick={() => setWaveform(mode.id, undefined)}
                  title="Remove waveform from this mode"
                >
                  Remove
                </button>
              )}
              {mode.waveformId && (
                <button
                  className="px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-white text-xs"
                  onClick={() => navigate(`/create/wave?select=${mode.waveformId}`)}
                  title="Edit selected waveform"
                >
                  Edit
                </button>
              )}
            </div>
            {selectedWaveform && (
              <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
                <WaveformMini wf={selectedWaveform} height={64} />
              </div>
            )}
          </div>

          {/* Case light color */}
          <div>
            <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">Case Light Color</div>
            <div className="flex items-center gap-3">
              <input
                type="checkbox"
                aria-label="Enable case light"
                title="Enable case light"
                checked={mode.color !== '#000000'}
                onChange={e => setColor(mode.id, e.target.checked ? '#3584e4' : '#000000')}
                className="accent-fg-ring"
              />
              <input
                type="color"
                value={mode.color}
                onChange={e => setColor(mode.id, e.target.value)}
                disabled={mode.color === '#000000'}
                className="h-8 w-12 p-0 bg-transparent border border-slate-700/50 rounded disabled:opacity-50"
                aria-label="Case light color"
                title={mode.color === '#000000' ? 'Enable case light to choose color' : 'Case light color'}
              />
            </div>
          </div>

          {/* Accelerometer section */}
          <div className="mt-3">
            <div className="flex items-center justify-between">
              <div className="text-xs uppercase tracking-wide text-slate-400">Accelerometer</div>
            </div>

            {triggers.length > 0 && (
              <div className="mt-2 space-y-2">
                {triggers.map((t, i) => {
                  const accelWf = t.waveformId ? waveforms.find(w => w.id === t.waveformId) ?? null : null;
                  const ALLOWED = [2, 4, 8, 12, 16];
                  const prevThresh = i > 0 ? triggers[i - 1]?.threshold : undefined;
                  const allowedAfterPrev = prevThresh == null ? ALLOWED : ALLOWED.filter(v => v > prevThresh);
                  return (
                    <div key={i} className="grid grid-cols-[auto_1fr_auto] items-center gap-2">
                      <div className="text-xs text-slate-400">Threshold (× g)</div>
                      <select
                        value={t.threshold}
                        onChange={e => setAccelTriggerThreshold(mode.id, i, Number(e.target.value))}
                        className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
                        aria-label="Acceleration threshold in multiples of g"
                      >
                        {allowedAfterPrev.map(v => (
                          <option key={v} value={v}>{`${v} g`}</option>
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
                      <div className="flex items-center gap-2 justify-self-end ml-3 sm:ml-4">
                        <input
                          type="checkbox"
                          aria-label="Enable trigger color"
                          title="Enable trigger color"
                          checked={(t.color ?? mode.color) !== '#000000'}
                          onChange={e => setAccelTriggerColor(mode.id, i, e.target.checked ? '#3584e4' : '#000000')}
                          className="accent-fg-ring"
                        />
                        <input
                          type="color"
                          value={t.color ?? mode.color}
                          onChange={e => setAccelTriggerColor(mode.id, i, e.target.value)}
                          disabled={(t.color ?? mode.color) === '#000000'}
                          className="h-8 w-12 p-0 bg-transparent border border-slate-700/50 rounded disabled:opacity-50"
                          aria-label="Trigger color"
                          title={(t.color ?? mode.color) === '#000000' ? 'Enable trigger color to choose color' : 'Trigger color'}
                        />
                      </div>

                      {accelWf && (
                        <div className="col-span-3">
                          <div className="mt-2 rounded border border-slate-700/50 bg-slate-900/60">
                            <WaveformMini wf={accelWf} height={56} />
                          </div>
                        </div>
                      )}
                    </div>
                  );
                })}
              </div>
            )}

            {triggers.length < 2 && (
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
      </div>

      {/* Export JSON */}
      <div className="mt-4">
        <div className="flex items-center justify-between mb-2">
          <div className="text-xs uppercase tracking-wide text-slate-400">Export (JSON)</div>
          <button
            className="px-2 py-1 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-xs"
            onClick={() => navigator.clipboard.writeText(exportModeAsJSON(mode.id))}
          >
            Copy
          </button>
        </div>
        <pre className="rounded border border-slate-700/50 bg-slate-900/60 p-2 text-xs overflow-x-auto">
{exportModeAsJSON(mode.id)}
        </pre>
      </div>

      {/* New Waveform Modal */}
      <Modal
        open={wfModalOpen}
        onClose={() => setWfModalOpen(false)}
        title="New Waveform"
        size="lg"
        footer={
          <>
            <button
              className="px-3 py-1.5 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-sm"
              onClick={() => setWfModalOpen(false)}
            >
              Cancel
            </button>
            <button
              className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white text-sm"
              onClick={() => {
                // Save draft first so it exists in library, select it, then navigate to full editor
                const id = addWaveform(wfDraft);
                setWaveform(mode.id, id);
                setWfModalOpen(false);
                navigate(`/create/wave?select=${id}`);
              }}
            >
              Edit Fullscreen
            </button>
            <button
              className="px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900 text-sm disabled:opacity-50"
              disabled={!canSaveDraft}
              onClick={() => {
                const id = addWaveform(wfDraft);
                setWaveform(mode.id, id);
                setWfModalOpen(false);
              }}
            >
              Save and Use
            </button>
          </>
        }
      >
        <div className="space-y-3">
          <div className="flex items-center gap-2">
            <label className="text-sm">Name</label>
            <input
              value={wfDraft.name}
              onChange={e => setWfDraft({ ...wfDraft, name: e.target.value })}
              className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
            />
            <label className="text-sm ml-4">Total Ticks</label>
            <input
              type="number"
              min={2}
              value={wfDraft.totalTicks}
              onChange={e => setWfDraft({ ...wfDraft, totalTicks: Math.max(2, Number(e.target.value)) })}
              className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
            />
          </div>
          {/* Inline editor */}
          <WaveformEditor value={wfDraft} onChange={setWfDraft} height={140} />
        </div>
      </Modal>

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
