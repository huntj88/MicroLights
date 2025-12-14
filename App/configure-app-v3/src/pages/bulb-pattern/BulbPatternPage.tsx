import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import {
  isBinaryPattern,
  simplePatternSchema,
  type SimplePattern,
} from '../../app/models/mode';
import { usePatternStore } from '../../app/providers/pattern-store';
import {
  type SimpleBulbPatternAction,
  SimpleBulbPatternPanel,
} from '../../components/bulb-pattern/SimpleBulbPatternPanel';
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

  const handlePatternChange = (
    nextPattern: SimplePattern,
    action: SimpleBulbPatternAction,
  ) => {
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

  const handlePatternSelect = (event: React.ChangeEvent<HTMLSelectElement>) => {
    const name = event.target.value;
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
        !window.confirm(
          t('rgbPattern.simple.storage.overwriteConfirm', { name: patternState.name }),
        )
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

    if (
      !window.confirm(
        t('rgbPattern.simple.storage.deleteConfirm', { name: selectedPatternName }),
      )
    ) {
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

      <article className="space-y-6">
        <header className="flex flex-wrap items-end justify-between gap-4 border-b theme-border pb-6">
          <div className="flex flex-1 flex-wrap items-end gap-4">
            <label className="flex flex-col gap-1.5">
              <span className="text-sm font-medium text-[rgb(var(--surface-contrast)/0.8)]">
                {t('rgbPattern.simple.storage.selectLabel')}
              </span>
              <select
                className="h-10 rounded-xl border theme-border bg-[rgb(var(--surface-raised)/0.5)] px-3 text-sm text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none"
                onChange={handlePatternSelect}
                value={selectedPatternName}
              >
                <option value="">{t('rgbPattern.simple.storage.selectPlaceholder')}</option>
                {availablePatternNames.map(name => (
                  <option key={name} value={name}>
                    {name}
                  </option>
                ))}
              </select>
            </label>

            <div className="flex gap-2">
              <button
                className="rounded-full px-4 py-2 text-sm font-medium text-red-400 transition-colors hover:bg-red-500/10 disabled:opacity-50"
                disabled={!selectedPatternName}
                onClick={handlePatternDelete}
                type="button"
              >
                {t('rgbPattern.simple.storage.deleteButton')}
              </button>
              <button
                className="rounded-full bg-[rgb(var(--accent)/1)] px-4 py-2 text-sm font-medium text-[rgb(var(--surface-contrast)/1)] transition-transform hover:scale-[1.01] disabled:opacity-50"
                disabled={!isDirty || validationErrors.length > 0}
                onClick={handlePatternSave}
                type="button"
              >
                {t('rgbPattern.simple.storage.saveButton')}
              </button>
            </div>
          </div>
        </header>

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
