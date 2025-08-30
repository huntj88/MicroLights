import { useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router-dom';

import { AccelTriggerRow } from '@/components/AccelTriggerRow';
import { CaseLightColorTogglePicker } from '@/components/CaseLightColorTogglePicker';
import { FingerSelector } from '@/components/FingerSelector';
import { WaveformEditorModal } from '@/components/WaveformEditorModal';
import { WaveformPicker } from '@/components/WaveformPicker';
import { DISABLED_COLOR } from '@/lib/constants';
import { useAppStore, type Mode } from '@/lib/store';
import type { Waveform } from '@/lib/waveform';

export function ModeCard({
  mode,
  showFingerOptions = true,
}: {
  mode: Mode;
  showFingerOptions?: boolean;
}) {
  const navigate = useNavigate();
  const { t } = useTranslation();
  const owner = useAppStore(s => s.fingerOwner);
  const setColor = useAppStore(s => s.setColor);
  const selectAll = useAppStore(s => s.selectAll);
  const assign = useAppStore(s => s.assignFinger);
  const unassign = useAppStore(s => s.unassignFinger);
  const remove = useAppStore(s => s.removeMode);

  const waveforms = useAppStore(s => s.waveforms);
  const setWaveform = useAppStore(s => s.setWaveform);
  const addWaveform = useAppStore(s => s.addWaveform);
  const updateWaveform = useAppStore(s => s.updateWaveform);

  // accelerometer actions
  const addAccelTrigger = useAppStore(s => s.addAccelTrigger);
  const removeAccelTrigger = useAppStore(s => s.removeAccelTrigger);
  const setAccelTriggerThreshold = useAppStore(s => s.setAccelTriggerThreshold);
  const setAccelTriggerWaveform = useAppStore(s => s.setAccelTriggerWaveform);
  const setAccelTriggerColor = useAppStore(s => s.setAccelTriggerColor);

  const exportModeAsJSON = useAppStore(s => s.exportModeAsJSON);

  // Type-safe triggers list
  type Trigger = NonNullable<Mode['accel']>['triggers'][number];
  const triggers: Trigger[] = mode.accel?.triggers ?? [];

  // Popup state for creating a new waveform inline
  const [wfModalOpen, setWfModalOpen] = useState(false);
  const [wfDraft, setWfDraft] = useState<Waveform>({
    name: 'New Wave',
    totalTicks: 16,
    changeAt: [{ tick: 0, output: 'high' }],
  });
  const [wfModalTarget, setWfModalTarget] = useState<
    { kind: 'mode' } | { kind: 'accel'; index: number }
  >({ kind: 'mode' });
  const [wfEditId, setWfEditId] = useState<string | null>(null);
  const canSaveDraft = useMemo(
    () =>
      wfDraft.name.trim().length > 0 &&
      wfDraft.totalTicks >= 2 &&
      wfDraft.changeAt.length > 0 &&
      wfDraft.changeAt[0]?.tick === 0,
    [wfDraft],
  );

  return (
    <div className="rounded-xl border border-slate-700/50 bg-bg-card p-4">
      <div className="grid grid-cols-1 gap-4">
        <div className="space-y-3">
          {showFingerOptions && (
            <div>
              <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">
                {t('fingers')}
              </div>
              <div className="flex gap-2 mb-2">
                <button
                  className="px-2 py-1 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-xs"
                  onClick={() => selectAll(mode.id)}
                >
                  {t('selectAll', { defaultValue: 'All' })}
                </button>
              </div>
              <FingerSelector
                modeId={mode.id}
                owner={owner}
                onToggle={(f, isOwned) => (isOwned ? unassign(mode.id, f) : assign(mode.id, f))}
              />
            </div>
          )}

          {showFingerOptions && <div className="h-px bg-slate-700/40 my-3" />}

          <div>
            <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">
              {t('waveform')}
            </div>
            <WaveformPicker
              value={mode.waveformId}
              onChange={id => setWaveform(mode.id, id)}
              waveforms={waveforms}
              onCreate={() => {
                setWfDraft({
                  name: 'New Wave',
                  totalTicks: 16,
                  changeAt: [{ tick: 0, output: 'high' }],
                });
                setWfEditId(null);
                setWfModalTarget({ kind: 'mode' });
                setWfModalOpen(true);
              }}
              onEdit={id => {
                const wf = waveforms.find(w => w.id === id);
                if (!wf) return;
                setWfDraft({ name: wf.name, totalTicks: wf.totalTicks, changeAt: wf.changeAt });
                setWfEditId(wf.id);
                setWfModalTarget({ kind: 'mode' });
                setWfModalOpen(true);
              }}
              showPreview
              previewHeight={64}
            />
          </div>

          {/* Case light color */}
          <div>
            <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">
              {t('caseLightColor')}
            </div>
            <CaseLightColorTogglePicker
              color={mode.color}
              onToggle={enabled => setColor(mode.id, enabled ? '#3584e4' : DISABLED_COLOR)}
              onChange={hex => setColor(mode.id, hex)}
            />
          </div>

          {/* Accelerometer section */}
          <div className="mt-3">
            <div className="flex items-center justify-between">
              <div className="text-xs uppercase tracking-wide text-slate-400">
                {t('accelerometer')}
              </div>
            </div>

            {triggers.length > 0 && (
              <div className="mt-2 space-y-2">
                {triggers.map((t, i) => (
                  <AccelTriggerRow
                    key={i}
                    trigger={t}
                    prevThreshold={i > 0 ? triggers[i - 1]?.threshold : undefined}
                    waveforms={waveforms}
                    defaultColor={mode.color}
                    onChangeThreshold={v => setAccelTriggerThreshold(mode.id, i, v)}
                    onRemove={() => removeAccelTrigger(mode.id, i)}
                    onChangeWaveform={id => setAccelTriggerWaveform(mode.id, i, id)}
                    onCreateWaveform={() => {
                      setWfDraft({
                        name: 'New Wave',
                        totalTicks: 16,
                        changeAt: [{ tick: 0, output: 'high' }],
                      });
                      setWfEditId(null);
                      setWfModalTarget({ kind: 'accel', index: i });
                      setWfModalOpen(true);
                    }}
                    onEditWaveform={id => {
                      const wf = waveforms.find(w => w.id === id);
                      if (!wf) return;
                      setWfDraft({
                        name: wf.name,
                        totalTicks: wf.totalTicks,
                        changeAt: wf.changeAt,
                      });
                      setWfEditId(wf.id);
                      setWfModalTarget({ kind: 'accel', index: i });
                      setWfModalOpen(true);
                    }}
                    onToggleColor={enabled =>
                      setAccelTriggerColor(mode.id, i, enabled ? '#3584e4' : DISABLED_COLOR)
                    }
                    onColorChange={hex => setAccelTriggerColor(mode.id, i, hex)}
                  />
                ))}
              </div>
            )}

            {triggers.length < 2 && (
              <div className="mt-2">
                <button
                  className="px-2 py-1 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-xs"
                  onClick={() => addAccelTrigger(mode.id)}
                >
                  {t('addTrigger')}
                </button>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Export JSON */}
      <div className="mt-4">
        <div className="flex items-center justify-between mb-2">
          <div className="text-xs uppercase tracking-wide text-slate-400">{t('exportJSON')}</div>
          <button
            className="px-2 py-1 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-xs"
            onClick={() => navigator.clipboard.writeText(exportModeAsJSON(mode.id))}
          >
            {t('copy')}
          </button>
        </div>
        <pre className="rounded border border-slate-700/50 bg-slate-900/60 p-2 text-xs overflow-x-auto">
          {exportModeAsJSON(mode.id)}
        </pre>
      </div>

      {/* New Waveform Modal */}
      <WaveformEditorModal
        open={wfModalOpen}
        title={wfEditId ? t('editWaveform') : t('newWaveform')}
        draft={wfDraft}
        editId={wfEditId}
        onClose={() => {
          setWfModalOpen(false);
          setWfEditId(null);
        }}
        onDraftChange={setWfDraft}
        canSave={canSaveDraft}
        onAction={action => {
          if (action === 'edit-fullscreen') {
            let id = wfEditId;
            if (id) {
              updateWaveform(id, wfDraft);
            } else {
              id = addWaveform(wfDraft);
              if (wfModalTarget.kind === 'mode') setWaveform(mode.id, id);
              else setAccelTriggerWaveform(mode.id, wfModalTarget.index, id);
            }
            setWfModalOpen(false);
            if (id) navigate(`/create/wave?select=${id}`);
            return;
          }
          if (action === 'save' || action === 'save-and-use') {
            if (wfEditId) {
              updateWaveform(wfEditId, wfDraft);
            } else {
              const id = addWaveform(wfDraft);
              if (wfModalTarget.kind === 'mode') setWaveform(mode.id, id);
              else setAccelTriggerWaveform(mode.id, wfModalTarget.index, id);
            }
            setWfModalOpen(false);
            setWfEditId(null);
          }
        }}
      />

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
