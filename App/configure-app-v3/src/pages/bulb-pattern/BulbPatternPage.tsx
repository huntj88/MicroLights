import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { isBinaryPattern, simplePatternSchema, type SimplePattern } from '../../app/models/mode';
import { usePatternStore } from '../../app/providers/pattern-store';
import {
  type SimpleBulbPatternAction,
  SimpleBulbPatternPanel,
} from '../../components/bulb-pattern/SimpleBulbPatternPanel';
import { StorageControls } from '../../components/common/StorageControls';
import { getLocalizedError } from '../../utils/localization';

const createEmptyPattern = (): SimplePattern => ({
  type: 'simple',
  name: '',
  duration: 0,
  changeAt: [],
});

export const BulbPatternPage = () => {
  const { t } = useTranslation();
  const [selectedPatternName, setSelectedPatternName] = useState('');
  const [patternState, setPatternState] = useState<SimplePattern>(createEmptyPattern);
  const [validationErrors, setValidationErrors] = useState<string[]>(() => {
    const initialPattern = createEmptyPattern();
    const result = simplePatternSchema.safeParse(initialPattern);
    return result.success ? [] : result.error.issues.map(issue => issue.message);
  });

  const patterns = usePatternStore(state => state.patterns);
  const savePattern = usePatternStore(state => state.savePattern);
  const deletePattern = usePatternStore(state => state.deletePattern);

  const availablePatternNames = useMemo(
    () =>
      patterns
        .filter(isBinaryPattern)
        .map(pattern => pattern.name)
        .sort((a, b) => a.localeCompare(b)),
    [patterns],
  );

  useEffect(() => {
    if (!selectedPatternName) {
      return;
    }

    if (availablePatternNames.includes(selectedPatternName)) {
      return;
    }

    if (patternState.name === selectedPatternName) {
      return;
    }

    setSelectedPatternName('');
  }, [availablePatternNames, selectedPatternName, patternState.name]);

  const handlePatternChange = (nextPattern: SimplePattern, action: SimpleBulbPatternAction) => {
    setPatternState(nextPattern);

    const result = simplePatternSchema.safeParse(nextPattern);
    if (!result.success) {
      setValidationErrors(result.error.issues.map(issue => issue.message));
    } else {
      setValidationErrors([]);
    }

    if (action.type === 'rename-pattern') {
      if (selectedPatternName === action.name) {
        return;
      }
      setSelectedPatternName('');
    }
  };

  const handlePatternSelect = (name: string) => {
    setSelectedPatternName(name);

    if (!name) {
      setPatternState(createEmptyPattern());
      setValidationErrors([]);
      return;
    }

    const pattern = patterns.find(p => p.name === name);
    if (pattern && isBinaryPattern(pattern)) {
      setPatternState(pattern);
      setValidationErrors([]);
    }
  };

  const handlePatternSave = () => {
    const result = simplePatternSchema.safeParse(patternState);
    if (!result.success) {
      return;
    }

    const existing = patterns.find(p => p.name === patternState.name);
    if (existing && existing.name !== selectedPatternName) {
      if (
        !window.confirm(t('patternEditor.storage.overwriteConfirm', { name: patternState.name }))
      ) {
        return;
      }
    }

    savePattern(patternState);
    setSelectedPatternName(patternState.name);
  };

  const handlePatternDelete = () => {
    if (!selectedPatternName) {
      return;
    }

    if (!window.confirm(t('patternEditor.storage.deleteConfirm', { name: selectedPatternName }))) {
      return;
    }

    deletePattern(selectedPatternName);
    setSelectedPatternName('');
    setPatternState(createEmptyPattern());
  };

  const isDirty = useMemo(() => {
    if (!selectedPatternName) {
      return patternState.changeAt.length > 0 || patternState.name.length > 0;
    }

    const original = patterns.find(p => p.name === selectedPatternName);
    return JSON.stringify(original) !== JSON.stringify(patternState);
  }, [patternState, selectedPatternName, patterns]);

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('bulbPattern.title')}</h2>
        <p className="theme-muted">{t('bulbPattern.subtitle')}</p>
      </header>

      <article className="space-y-6 rounded-2xl border border-dashed theme-border bg-[rgb(var(--surface-raised)/0.35)] p-6">
        <StorageControls
          items={availablePatternNames}
          selectedItem={selectedPatternName}
          onSelect={handlePatternSelect}
          selectLabel={t('patternEditor.storage.selectLabel')}
          selectPlaceholder={t('patternEditor.storage.selectPlaceholder')}
          onSave={handlePatternSave}
          onDelete={handlePatternDelete}
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

        <SimpleBulbPatternPanel onChange={handlePatternChange} value={patternState} />
      </article>
    </section>
  );
};
