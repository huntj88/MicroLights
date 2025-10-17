import { useTranslation } from 'react-i18next';

import { Modal } from '@/components/Modal';
import { BulbModeWaveformEditor } from '@/components/BulbModeWaveformEditor';
import type { BulbModeWaveform } from '@/lib/bulbModeWaveform';

export type BulbModeWaveformEditorAction = 'edit-fullscreen' | 'save' | 'save-and-use';

export function BulbModeWaveformEditorModal({
  open,
  title,
  draft,
  editId,
  onClose,
  onAction,
  onDraftChange,
  canSave,
}: {
  open: boolean;
  title: string;
  draft: BulbModeWaveform;
  editId?: string | null;
  onClose: () => void;
  onAction: (action: BulbModeWaveformEditorAction) => void;
  onDraftChange: (wf: BulbModeWaveform) => void;
  canSave: boolean;
}) {
  const { t } = useTranslation();
  return (
    <Modal
      open={open}
      onClose={onClose}
      title={title}
      size="lg"
      footer={
        <>
          <button
            className="px-3 py-1.5 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-sm"
            onClick={onClose}
          >
            {t('cancel')}
          </button>
          <button
            className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white text-sm"
            onClick={() => onAction('edit-fullscreen')}
            disabled={editId == null}
          >
            {t('editFullscreen')}
          </button>
          <button
            className="px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900 text-sm disabled:opacity-50"
            disabled={!canSave}
            onClick={() => onAction(editId ? 'save' : 'save-and-use')}
          >
            {editId ? t('save') : t('saveAndUse')}
          </button>
        </>
      }
    >
      <div className="space-y-3">
        <div className="flex items-center gap-2">
          <label className="text-sm">{t('name')}</label>
          <input
            value={draft.name}
            onChange={e => onDraftChange({ ...draft, name: e.target.value })}
            className="bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
          />
          <label className="text-sm ml-4">{t('totalTicks')}</label>
          <input
            type="number"
            min={2}
            value={draft.totalTicks}
            onChange={e =>
              onDraftChange({ ...draft, totalTicks: Math.max(2, Number(e.target.value)) })
            }
            className="w-24 bg-transparent border border-slate-700/50 rounded px-2 py-1 text-sm"
          />
        </div>
        <BulbModeWaveformEditor value={draft} onChange={onDraftChange} height={140} readOnly={false} />
      </div>
    </Modal>
  );
}
