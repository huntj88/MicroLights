import { useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router-dom';

import { AccelTriggerRow } from '@/components/AccelTriggerRow';
import { CaseLightColorTogglePicker } from '@/components/CaseLightColorTogglePicker';
import { CloseButton } from '@/components/CloseButton';
import { FingerSelector } from '@/components/FingerSelector';
import { BulbModeWaveformEditorModal } from '@/components/BulbModeWaveformEditorModal';
import { BulbModeWaveformPicker } from '@/components/BulbModeWaveformPicker';
import { DISABLED_COLOR } from '@/lib/constants';
import { DEFAULT_NEW_BULB_MODE_WAVEFORM } from '@/lib/defaultWaveforms';
import { Trigger, useAppStore, type Mode } from '@/lib/store';
import type { BulbModeWaveform } from '@/lib/bulbModeWaveform';

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

  const bulbModeWaveforms = useAppStore(s => s.bulbModeWaveforms);
  const setBulbModeWaveformId = useAppStore(s => s.setBulbModeWaveformId);
  const addBulbModeWaveform = useAppStore(s => s.addBulbModeWaveform);
  const updateBulbModeWaveform = useAppStore(s => s.updateBulbModeWaveform);

  // accelerometer actions
  const addAccelTrigger = useAppStore(s => s.addAccelTrigger);
  const removeAccelTrigger = useAppStore(s => s.removeAccelTrigger);
  const setAccelTriggerThreshold = useAppStore(s => s.setAccelTriggerThreshold);
  const setAccelTriggerBulbModeWaveformId = useAppStore(
    s => s.setAccelTriggerBulbModeWaveformId,
  );
  const setAccelTriggerColor = useAppStore(s => s.setAccelTriggerColor);

  const exportModeAsJSON = useAppStore(s => s.exportModeAsJSON);

  const triggers: Trigger[] = mode.accel?.triggers ?? [];

  // Popup state for creating a new waveform inline
  const [wfModalOpen, setWfModalOpen] = useState(false);
  const [wfDraft, setWfDraft] = useState<BulbModeWaveform>(DEFAULT_NEW_BULB_MODE_WAVEFORM);
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
    <div className="relative rounded-xl border border-slate-700/50 bg-bg-card p-4">
      {/* Top-right remove button */}
      <CloseButton className="absolute top-2 right-2" onClick={() => remove(mode.id)} />
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
            <BulbModeWaveformPicker
              value={mode.bulbModeWaveformId}
              onChange={(id: string | undefined) => setBulbModeWaveformId(mode.id, id)}
              waveforms={bulbModeWaveforms}
              onCreate={() => {
                setWfDraft(DEFAULT_NEW_BULB_MODE_WAVEFORM);
                setWfEditId(null);
                setWfModalTarget({ kind: 'mode' });
                setWfModalOpen(true);
              }}
              onEdit={(id: string) => {
                const wf = bulbModeWaveforms.find(w => w.id === id);
                if (!wf) return;
                setWfDraft({ name: wf.name, totalTicks: wf.totalTicks, changeAt: wf.changeAt });
                // If readonly, force Save-As (no edit id)
                setWfEditId(wf.readonly ? null : wf.id);
                setWfModalTarget({ kind: 'mode' });
                setWfModalOpen(true);
              }}
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
              <div className="mt-2 space-y-3">
                {triggers.map((trig, i) => (
                  <AccelTriggerRow
                    key={i}
                    index={i}
                    trigger={trig}
                    prevThreshold={i > 0 ? triggers[i - 1]?.threshold : undefined}
                    waveforms={bulbModeWaveforms}
                    defaultColor={mode.color}
                    onChangeThreshold={v => setAccelTriggerThreshold(mode.id, i, v)}
                    onRemove={() => removeAccelTrigger(mode.id, i)}
                    onChangeWaveform={(id: string | undefined) =>
                      setAccelTriggerBulbModeWaveformId(mode.id, i, id)
                    }
                    onCreateWaveform={() => {
                      setWfDraft(DEFAULT_NEW_BULB_MODE_WAVEFORM);
                      setWfEditId(null);
                      setWfModalTarget({ kind: 'accel', index: i });
                      setWfModalOpen(true);
                    }}
                    onEditWaveform={id => {
                      const wf = bulbModeWaveforms.find(w => w.id === id);
                      if (!wf) return;
                      setWfDraft({
                        name: wf.name,
                        totalTicks: wf.totalTicks,
                        changeAt: wf.changeAt,
                      });
                      setWfEditId(wf.readonly ? null : wf.id);
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
      <BulbModeWaveformEditorModal
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
              updateBulbModeWaveform(id, wfDraft);
            } else {
              id = addBulbModeWaveform(wfDraft);
              if (wfModalTarget.kind === 'mode') setBulbModeWaveformId(mode.id, id);
              else setAccelTriggerBulbModeWaveformId(mode.id, wfModalTarget.index, id);
            }
            setWfModalOpen(false);
            if (id) navigate(`/create/wave?select=${id}`);
            return;
          }
          if (action === 'save' || action === 'save-and-use') {
            if (wfEditId) {
              updateBulbModeWaveform(wfEditId, wfDraft);
            } else {
              const id = addBulbModeWaveform(wfDraft);
              if (wfModalTarget.kind === 'mode') setBulbModeWaveformId(mode.id, id);
              else setAccelTriggerBulbModeWaveformId(mode.id, wfModalTarget.index, id);
            }
            setWfModalOpen(false);
            setWfEditId(null);
          }
        }}
      />
    </div>
  );
}
