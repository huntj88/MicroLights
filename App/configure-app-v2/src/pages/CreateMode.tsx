import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { toast } from 'sonner';

import { ModeCard } from '@/components/ModeCard';
import { useAppStore } from '@/lib/store';
import { DEFAULT_NEW_WAVEFORM } from '@/lib/defaultWaveforms';

export default function CreateMode() {
  const { t } = useTranslation();
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
  const lastSelectedModeSetId = useAppStore(s => s.lastSelectedModeSetId);

  const [selectedSetId, setSelectedSetId] = useState<string>('');
  const [draftName, setDraftName] = useState<string>('');

  const canAdd = useMemo(() => modes.length < 10, [modes.length]);
  const selectedSet = useMemo(
    () => modeSets.find(s => s.id === selectedSetId),
    [modeSets, selectedSetId],
  );

  // On first mount or when library changes, auto-load the most recent selection
  useEffect(() => {
    // If local selection is already set, keep it in sync if it exists; otherwise choose from store
    const fromStore = lastSelectedModeSetId ?? '';
    if (!selectedSetId) {
      const candidate = fromStore || (modeSets.length > 0 ? modeSets[modeSets.length - 1]!.id : '');
      if (candidate) {
        setSelectedSetId(candidate);
        // Load only if current modes aren't already tied to this set
        loadModeSet(candidate);
        const name = modeSets.find(s => s.id === candidate)?.name ?? '';
        setDraftName(name);
        // avoid toast spam on initial load
      } else {
        // no modeset yet; make sure draft has a sensible default name
        setDraftName(DEFAULT_NEW_WAVEFORM.name);
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [modeSets.length]);

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold">{t('createModeTitle')}</h1>
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
              <option value="" disabled={!!selectedSetId}>
                {t('unsavedDraft')}
              </option>
              {modeSets.map(s => (
                <option key={s.id} value={s.id}>
                  {s.name}
                </option>
              ))}
            </select>
            <button
              type="button"
              onClick={() => {
                newModeSetDraft();
                setSelectedSetId('');
                setDraftName(DEFAULT_NEW_WAVEFORM.name);
              }}
              className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white"
            >
              {t('newDraft')}
            </button>
            {selectedSet && (
              <button
                type="button"
                onClick={() => {
                  if (!selectedSetId) return;
                  if (confirm(t('confirmDeleteMode'))) {
                    removeModeSet(selectedSetId);
                    setSelectedSetId('');
                    newModeSetDraft();
                    setDraftName('');
                  }
                }}
                className="px-3 py-1.5 rounded bg-red-600 hover:bg-red-500 text-white"
              >
                {t('delete')}
              </button>
            )}
            <button
              type="button"
              onClick={() => {
                if (selectedSet) {
                  updateModeSet(selectedSet.id, draftName.trim() || selectedSet.name);
                  toast.success(t('modeSaved'));
                } else {
                  const id = saveCurrentModeSet(draftName);
                  setSelectedSetId(id);
                  toast.success(t('addedToLibrary'));
                }
              }}
              className="px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900"
            >
              {selectedSet ? t('save') : t('addToLibrary')}
            </button>
          </div>

          <button
            type="button"
            onClick={() => (connected ? disconnect() : connect())}
            className={`px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white`}
          >
            {connected ? t('disconnect') : t('connect')}
          </button>
          <button
            type="button"
            onClick={async () => {
              await send();
              toast.success(t('programSent'));
            }}
            className="px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900"
          >
            {t('send')}
          </button>
        </div>
      </div>

      {/* Name row aligned like Waveform page */}
      <div className="flex items-center gap-2">
        <label className="text-sm">{t('name')}</label>
        <input
          value={draftName}
          onChange={e => setDraftName(e.target.value)}
          className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
        />
      </div>

      <div className="flex items-center gap-2">
        <div className="text-xs text-slate-400">
          {t('tip')} {t('tipModeSave')}
        </div>
      </div>

      <div className="flex items-center gap-2">
        <div className="text-xs text-slate-400">
          {t('tip')} {modes.length > 1 ? t('modeCardTipPlural') : t('modeCardTipSingle')}
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
          title={canAdd ? t('addNewMode') : t('maxModesReached')}
        >
          {t('addModeCard')}
        </button>
      </div>
    </div>
  );
}
