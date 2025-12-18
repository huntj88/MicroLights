import { useCallback, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import {
  createDefaultEquationPattern,
  equationPatternSchema,
  isColorPattern,
  simplePatternSchema,
  type EquationPattern,
  type SimplePattern,
} from '../../app/models/mode';
import { usePatternStore } from '../../app/providers/pattern-store';
import { SerialConnectButton } from '../../components/common/SerialConnectButton';
import { SerialTestButton } from '../../components/common/SerialTestButton';
import { StorageControls } from '../../components/common/StorageControls';
import {
  type EquationRgbPatternAction,
  EquationRgbPatternPanel,
} from '../../components/pattern/rgb/equation/EquationRgbPatternPanel';
import {
  type SimpleRgbPatternAction,
  SimpleRgbPatternPanel,
} from '../../components/pattern/rgb/SimpleRgbPatternPanel';
import { useEntityEditor } from '../../hooks/useEntityEditor';
import { getLocalizedError } from '../../utils/localization';

const createEmptyPattern = (): SimplePattern => ({
  type: 'simple',
  name: '',
  duration: 0,
  changeAt: [],
});

const validateSimple = (pattern: SimplePattern) => {
  const result = simplePatternSchema.safeParse(pattern);
  return result.success ? [] : result.error.issues.map(issue => issue.message);
};

const validateEquation = (pattern: EquationPattern) => {
  const result = equationPatternSchema.safeParse(pattern);
  return result.success ? [] : result.error.issues.map(issue => issue.message);
};

export const RgbPatternPage = () => {
  const { t } = useTranslation();
  const [activeMethod, setActiveMethod] = useState<'simple' | 'equation'>('simple');

  const patterns = usePatternStore(state => state.patterns);
  const savePattern = usePatternStore(state => state.savePattern);
  const deletePattern = usePatternStore(state => state.deletePattern);
  const getPattern = usePatternStore(state => state.getPattern);

  const simplePatternNames = useMemo(
    () =>
      patterns
        .filter(isColorPattern)
        .map(p => p.name)
        .sort((a, b) => a.localeCompare(b)),
    [patterns],
  );

  const equationPatternNames = useMemo(
    () =>
      patterns
        .filter(p => p.type === 'equation')
        .map(p => p.name)
        .sort((a, b) => a.localeCompare(b)),
    [patterns],
  );

  const readSimpleItem = useCallback(
    (name: string) => {
      const p = getPattern(name);
      if (!p) return undefined;
      return isColorPattern(p) ? p : undefined;
    },
    [getPattern],
  );

  const readEquationItem = useCallback(
    (name: string) => {
      const p = getPattern(name);
      if (!p) return undefined;
      return p.type === 'equation' ? p : undefined;
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

  const simpleEditor = useEntityEditor<SimplePattern>({
    availableNames: simplePatternNames,
    readItem: readSimpleItem,
    saveItem: savePattern,
    deleteItem: deletePattern,
    createDefault: createEmptyPattern,
    validate: validateSimple,
    confirmOverwrite,
    confirmDelete,
  });

  const equationEditor = useEntityEditor<EquationPattern>({
    availableNames: equationPatternNames,
    readItem: readEquationItem,
    saveItem: savePattern,
    deleteItem: deletePattern,
    createDefault: createDefaultEquationPattern,
    validate: validateEquation,
    confirmOverwrite,
    confirmDelete,
  });

  const currentEditor = activeMethod === 'simple' ? simpleEditor : equationEditor;
  const currentAvailableNames =
    activeMethod === 'simple' ? simplePatternNames : equationPatternNames;

  const handleSimplePatternChange = (
    nextPattern: SimplePattern,
    action: SimpleRgbPatternAction,
  ) => {
    console.log('RGB simple pattern change:', action, nextPattern);
    simpleEditor.setEditingItem(nextPattern);
  };

  const handleEquationPatternChange = (
    nextPattern: EquationPattern,
    action: EquationRgbPatternAction,
  ) => {
    console.log('RGB equation pattern change:', action, nextPattern);
    equationEditor.setEditingItem(nextPattern);
  };

  const isSimpleMethod = activeMethod === 'simple';

  return (
    <section className="space-y-6">
      <header className="flex flex-wrap items-start justify-between gap-4">
        <div className="space-y-2">
          <h2 className="text-3xl font-semibold">{t('rgbPattern.title')}</h2>
          <p className="theme-muted">{t('rgbPattern.subtitle')}</p>
        </div>
        <div className="flex items-center gap-2">
          <SerialTestButton
            data={currentEditor.editingItem}
            type="pattern"
            patternTarget="case"
            disabled={!currentEditor.isValid}
          />
          <SerialConnectButton />
        </div>
      </header>
      <div className="flex flex-wrap items-center gap-3">
        <span className="text-sm font-medium">{t('rgbPattern.methodSwitcher.label')}</span>
        <div className="theme-border flex overflow-hidden rounded-full border">
          <button
            className={`px-4 py-2 text-sm transition-colors ${
              isSimpleMethod
                ? 'bg-[rgb(var(--accent)/0.25)] text-[rgb(var(--accent-contrast)/1)]'
                : 'theme-muted hover:bg-[rgb(var(--surface-raised)/0.5)]'
            }`}
            onClick={() => {
              setActiveMethod('simple');
            }}
            type="button"
          >
            {t('rgbPattern.simple.title')}
          </button>
          <button
            className={`px-4 py-2 text-sm transition-colors ${
              !isSimpleMethod
                ? 'bg-[rgb(var(--accent)/0.25)] text-[rgb(var(--accent-contrast)/1)]'
                : 'theme-muted hover:bg-[rgb(var(--surface-raised)/0.5)]'
            }`}
            onClick={() => {
              setActiveMethod('equation');
            }}
            type="button"
          >
            {t('rgbPattern.methodSwitcher.equation')}
          </button>
        </div>
      </div>

      <article
        className={`space-y-6 rounded-2xl border border-dashed theme-border bg-[rgb(var(--surface-raised)/0.35)] p-6`}
      >
        <header className="space-y-3">
          <div className="space-y-1">
            <h3 className="text-2xl font-semibold">
              {isSimpleMethod ? t('rgbPattern.simple.title') : t('rgbPattern.equation.title')}
            </h3>
            <p className="theme-muted text-sm">
              {isSimpleMethod
                ? t('rgbPattern.simple.description')
                : t('rgbPattern.equation.description')}
            </p>
          </div>
          <StorageControls
            items={currentAvailableNames}
            selectedItem={currentEditor.selectedName}
            onSelect={currentEditor.setSelectedName}
            selectLabel={t('patternEditor.storage.selectLabel')}
            selectPlaceholder={t('patternEditor.storage.selectPlaceholder')}
            onSave={currentEditor.save}
            onDelete={currentEditor.remove}
            isDirty={currentEditor.isDirty}
            isValid={currentEditor.isValid}
            saveLabel={t('patternEditor.storage.saveButton')}
            deleteLabel={t('patternEditor.storage.deleteButton')}
          />
        </header>

        {currentEditor.validationErrors.length > 0 && (
          <div className="rounded-xl border border-red-500/50 bg-red-500/10 p-4 text-sm text-red-500">
            <ul className="list-inside list-disc space-y-1">
              {currentEditor.validationErrors.map((error, index) => (
                <li key={index}>{getLocalizedError(error, t)}</li>
              ))}
            </ul>
          </div>
        )}

        {isSimpleMethod ? (
          <SimpleRgbPatternPanel
            value={simpleEditor.editingItem}
            onChange={handleSimplePatternChange}
          />
        ) : (
          <EquationRgbPatternPanel
            pattern={equationEditor.editingItem}
            onChange={handleEquationPatternChange}
          />
        )}
      </article>
    </section>
  );
};
