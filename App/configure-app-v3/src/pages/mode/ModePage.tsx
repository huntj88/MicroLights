import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { hexColorSchema, type Mode } from '../../app/models/mode';
import { useModeStore } from '../../app/providers/mode-store';
import { usePatternStore } from '../../app/providers/pattern-store';
import { type ModeAction, ModeEditor } from '../../components/mode/ModeEditor';

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
      }
    } else {
      setEditingMode(createDefaultMode());
    }
  }, [selectedModeName, getMode]);

  const handleModeChange = (newMode: Mode, action: ModeAction) => {
    console.log('Mode changed:', action);
    setEditingMode(newMode);
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

  const isValid =
    editingMode.name.trim().length > 0 &&
    editingMode.front.pattern.name.length > 0 &&
    editingMode.case.pattern.name.length > 0;

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('mode.title')}</h2>
        <p className="theme-muted">{t('mode.subtitle')}</p>
      </header>

      <div className="theme-panel theme-border rounded-xl border p-6 space-y-4">
        <div className="flex items-end gap-4">
          <div className="flex-1 space-y-1">
            <label className="text-sm font-medium">{t('modeEditor.storage.selectLabel')}</label>
            <select
              className="theme-input w-full rounded-md border px-3 py-2"
              value={selectedModeName}
              onChange={e => {
                handleLoad(e.target.value);
              }}
            >
              <option value="">{t('modeEditor.storage.selectPlaceholder')}</option>
              {availableModeNames.map(name => (
                <option key={name} value={name}>
                  {name}
                </option>
              ))}
            </select>
          </div>
          {selectedModeName && (
            <button
              onClick={handleDelete}
              className="theme-button bg-red-500 hover:bg-red-600 text-white px-4 py-2 rounded-md"
            >
              {t('modeEditor.storage.delete')}
            </button>
          )}
        </div>
      </div>

      <ModeEditor mode={editingMode} onChange={handleModeChange} patterns={patterns} />

      <div className="flex justify-end">
        <button
          onClick={handleSave}
          disabled={!isValid}
          className="theme-button theme-button-primary px-4 py-2 rounded-md disabled:opacity-50 disabled:cursor-not-allowed"
        >
          {t('modeEditor.storage.save')}
        </button>
      </div>
    </section>
  );
};
