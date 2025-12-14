import { useMemo } from 'react';
import { useTranslation } from 'react-i18next';

import { type Mode, modeSchema } from '../../app/models/mode';
import { useModeStore } from '../../app/providers/mode-store';
import { usePatternStore } from '../../app/providers/pattern-store';
import { StorageControls } from '../../components/common/StorageControls';
import { type ModeAction, ModeEditor } from '../../components/mode/ModeEditor';
import { useEntityEditor } from '../../hooks/useEntityEditor';
import { getLocalizedError } from '../../utils/localization';

const createDefaultMode = (): Mode => ({
  name: '',
  front: undefined,
  case: undefined,
});

export const ModePage = () => {
  const { t } = useTranslation();
  const modes = useModeStore(state => state.modes);
  const saveMode = useModeStore(state => state.saveMode);
  const deleteMode = useModeStore(state => state.deleteMode);
  const getMode = useModeStore(state => state.getMode);
  const patterns = usePatternStore(state => state.patterns);

  const availableModeNames = useMemo(
    () => modes.map(m => m.name).sort((a, b) => a.localeCompare(b)),
    [modes],
  );

  const {
    selectedName: selectedModeName,
    setSelectedName: setSelectedModeName,
    editingItem: editingMode,
    setEditingItem: setEditingMode,
    validationErrors,
    isDirty,
    isValid,
    save: handleSave,
    remove: handleDelete,
  } = useEntityEditor<Mode>({
    availableNames: availableModeNames,
    readItem: getMode,
    saveItem: saveMode,
    deleteItem: deleteMode,
    createDefault: createDefaultMode,
    validate: mode => {
      const result = modeSchema.safeParse(mode);
      return result.success ? [] : result.error.issues.map(i => i.message);
    },
    confirmOverwrite: name => confirm(t('modeEditor.storage.overwriteConfirm', { name })),
    confirmDelete: () => confirm(t('modeEditor.storage.confirmDelete')),
  });

  const handleModeChange = (newMode: Mode, action: ModeAction) => {
    console.log('Mode changed:', action, newMode);
    setEditingMode(newMode);
  };

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('mode.title')}</h2>
        <p className="theme-muted">{t('mode.subtitle')}</p>
      </header>

      <div className="space-y-6 rounded-2xl border border-dashed theme-border bg-[rgb(var(--surface-raised)/0.35)] p-6">
        <StorageControls
          items={availableModeNames}
          selectedItem={selectedModeName}
          onSelect={setSelectedModeName}
          selectLabel={t('modeEditor.storage.selectLabel')}
          selectPlaceholder={t('modeEditor.storage.selectPlaceholder')}
          onSave={handleSave}
          onDelete={handleDelete}
          isDirty={isDirty}
          isValid={isValid}
          saveLabel={t('modeEditor.storage.save')}
          deleteLabel={t('modeEditor.storage.delete')}
        />

        {validationErrors.length > 0 && (
          <div className="rounded-xl border border-red-500/50 bg-red-500/10 p-4 text-sm text-red-500">
            <ul className="list-inside list-disc space-y-1">
              {validationErrors.map((error, index) => (
                <li key={index}>{getLocalizedError(error, t)}</li>
              ))}
            </ul>
          </div>
        )}

        <ModeEditor mode={editingMode} onChange={handleModeChange} patterns={patterns} />
      </div>
    </section>
  );
};
