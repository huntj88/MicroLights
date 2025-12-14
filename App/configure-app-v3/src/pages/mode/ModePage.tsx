import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { hexColorSchema, type Mode, modeSchema } from '../../app/models/mode';
import { useModeStore } from '../../app/providers/mode-store';
import { usePatternStore } from '../../app/providers/pattern-store';
import { StorageControls } from '../../components/common/StorageControls';
import { type ModeAction, ModeEditor } from '../../components/mode/ModeEditor';
import { getLocalizedError } from '../../utils/localization';

const createDefaultMode = (): Mode => ({
  name: '',
  front: {
    pattern: {
      type: 'simple',
      name: 'default-front',
      duration: 1000,
      changeAt: [{ ms: 0, output: 'low' }],
    },
  },
  case: {
    pattern: {
      type: 'simple',
      name: 'default-case',
      duration: 1000,
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#000000') }],
    },
  },
});

export const ModePage = () => {
  const { t } = useTranslation();
  const modes = useModeStore(state => state.modes);
  const saveMode = useModeStore(state => state.saveMode);
  const deleteMode = useModeStore(state => state.deleteMode);
  const getMode = useModeStore(state => state.getMode);
  const patterns = usePatternStore(state => state.patterns);

  const [selectedModeName, setSelectedModeName] = useState('');
  const [editingMode, setEditingMode] = useState<Mode>(createDefaultMode());
  const [originalMode, setOriginalMode] = useState<Mode | null>(null);
  const [validationErrors, setValidationErrors] = useState<string[]>(() => {
    const initial = createDefaultMode();
    const result = modeSchema.safeParse(initial);
    return result.success ? [] : result.error.issues.map(i => i.message);
  });

  const availableModeNames = useMemo(
    () => modes.map(m => m.name).sort((a, b) => a.localeCompare(b)),
    [modes],
  );

  // Load mode when selection changes
  useEffect(() => {
    if (selectedModeName) {
      const mode = getMode(selectedModeName);
      if (mode) {
        setEditingMode(mode);
        setOriginalMode(mode);
        setValidationErrors([]);
      }
    } else {
      const newMode = createDefaultMode();
      setEditingMode(newMode);
      setOriginalMode(null);
      const result = modeSchema.safeParse(newMode);
      setValidationErrors(result.success ? [] : result.error.issues.map(i => i.message));
    }
  }, [selectedModeName, getMode]);

  const handleModeChange = (newMode: Mode, action: ModeAction) => {
    console.log('Mode changed:', action);
    setEditingMode(newMode);

    const result = modeSchema.safeParse(newMode);
    setValidationErrors(result.success ? [] : result.error.issues.map(i => i.message));
  };

  const handleSave = () => {
    if (
      editingMode.name !== selectedModeName &&
      availableModeNames.includes(editingMode.name) &&
      !confirm(t('modeEditor.storage.overwriteConfirm', { name: editingMode.name }))
    ) {
      return;
    }
    saveMode(editingMode);
    setSelectedModeName(editingMode.name);
    setOriginalMode(editingMode);
  };

  const handleDelete = () => {
    if (!selectedModeName) return;
    if (confirm(t('modeEditor.storage.confirmDelete'))) {
      deleteMode(selectedModeName);
      setSelectedModeName('');
    }
  };

  const handleLoad = (name: string) => {
    setSelectedModeName(name);
  };

  const isValid = validationErrors.length === 0;

  const isDirty = useMemo(() => {
    if (!selectedModeName || !originalMode) {
      return true;
    }
    return JSON.stringify(editingMode) !== JSON.stringify(originalMode);
  }, [selectedModeName, originalMode, editingMode]);

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
          onSelect={handleLoad}
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
