import { useCallback, useMemo } from 'react';
import { useTranslation } from 'react-i18next';

import { isBinaryPattern, simplePatternSchema, type SimplePattern } from '../../app/models/mode';
import { usePatternStore } from '../../app/providers/pattern-store';
import { StorageControls } from '../../components/common/StorageControls';
import {
  type SimpleBulbPatternAction,
  SimpleBulbPatternPanel,
} from '../../components/pattern/bulb/SimpleBulbPatternPanel';
import { useEntityEditor } from '../../hooks/useEntityEditor';
import { getLocalizedError } from '../../utils/localization';

const createEmptyPattern = (): SimplePattern => ({
  type: 'simple',
  name: '',
  duration: 0,
  changeAt: [],
});

const validate = (pattern: SimplePattern) => {
  const result = simplePatternSchema.safeParse(pattern);
  return result.success ? [] : result.error.issues.map(issue => issue.message);
};

export const BulbPatternPage = () => {
  const { t } = useTranslation();

  const patterns = usePatternStore(state => state.patterns);
  const savePattern = usePatternStore(state => state.savePattern);
  const deletePattern = usePatternStore(state => state.deletePattern);
  const getPattern = usePatternStore(state => state.getPattern);

  const availablePatternNames = useMemo(
    () =>
      patterns
        .filter(isBinaryPattern)
        .map(pattern => pattern.name)
        .sort((a, b) => a.localeCompare(b)),
    [patterns],
  );

  const readItem = useCallback(
    (name: string) => {
      const p = getPattern(name);
      if (!p) return undefined;
      return isBinaryPattern(p) ? p : undefined;
    },
    [getPattern],
  );

  const confirmOverwrite = useCallback(
    (name: string) => confirm(t('patternEditor.storage.overwriteConfirm', { name })),
    [t],
  );

  const confirmDelete = useCallback(
    (name: string) => confirm(t('patternEditor.storage.deleteConfirm', { name })),
    [t],
  );

  const {
    selectedName,
    editingItem,
    validationErrors,
    isDirty,
    setSelectedName,
    setEditingItem: handleUpdate,
    save: handleSave,
    remove: handleDelete,
  } = useEntityEditor<SimplePattern>({
    availableNames: availablePatternNames,
    readItem,
    saveItem: savePattern,
    deleteItem: deletePattern,
    createDefault: createEmptyPattern,
    validate,
    confirmOverwrite,
    confirmDelete,
  });

  const handlePatternChange = (nextPattern: SimplePattern, action: SimpleBulbPatternAction) => {
    console.log('Bulb pattern change:', action, nextPattern);
    handleUpdate(nextPattern);
  };

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('bulbPattern.title')}</h2>
        <p className="theme-muted">{t('bulbPattern.subtitle')}</p>
      </header>

      <article className="space-y-6 rounded-2xl border border-dashed theme-border bg-[rgb(var(--surface-raised)/0.35)] p-6">
        <StorageControls
          items={availablePatternNames}
          selectedItem={selectedName}
          onSelect={setSelectedName}
          selectLabel={t('patternEditor.storage.selectLabel')}
          selectPlaceholder={t('patternEditor.storage.selectPlaceholder')}
          onSave={handleSave}
          onDelete={handleDelete}
          isDirty={isDirty}
          isValid={validationErrors.length === 0}
          saveLabel={t('patternEditor.storage.saveButton')}
          deleteLabel={t('patternEditor.storage.deleteButton')}
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

        <SimpleBulbPatternPanel onChange={handlePatternChange} value={editingItem} />
      </article>
    </section>
  );
};
