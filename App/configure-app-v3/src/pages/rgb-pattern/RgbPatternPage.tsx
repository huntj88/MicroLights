import type { ChangeEvent } from 'react';
import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import {
  createDefaultEquationPattern,
  equationPatternSchema,
  simplePatternSchema,
  type EquationPattern,
  type ModePattern,
  type SimplePattern,
} from '../../app/models/mode';
import { usePatternStore } from '../../app/providers/pattern-store';
import {
  type EquationRgbPatternAction,
  EquationRgbPatternPanel,
} from '../../components/rgb-pattern/equation/EquationRgbPatternPanel';
import {
  type SimpleRgbPatternAction,
  SimpleRgbPatternPanel,
} from '../../components/rgb-pattern/SimpleRgbPatternPanel';

const createEmptyPattern = (): SimplePattern => ({
  type: 'simple',
  name: '',
  duration: 0,
  changeAt: [],
});

export const RgbPatternPage = () => {
  const { t } = useTranslation();
  const [activeMethod, setActiveMethod] = useState<'simple' | 'equation'>('simple');
  const [selectedPatternName, setSelectedPatternName] = useState('');
  const [simplePatternState, setSimplePatternState] = useState<SimplePattern>(createEmptyPattern);
  const [equationPatternState, setEquationPatternState] = useState<EquationPattern>(
    createDefaultEquationPattern,
  );
  const [originalPattern, setOriginalPattern] = useState<ModePattern | null>(null);
  const [validationErrors, setValidationErrors] = useState<string[]>(() => {
    const initialPattern = createEmptyPattern();
    const result = simplePatternSchema.safeParse(initialPattern);
    return result.success ? [] : result.error.issues.map(issue => issue.message);
  });

  const patterns = usePatternStore(state => state.patterns);
  const savePattern = usePatternStore(state => state.savePattern);
  const deletePattern = usePatternStore(state => state.deletePattern);
  const getPattern = usePatternStore(state => state.getPattern);

  const availablePatternNames = useMemo(
    () =>
      patterns
        .filter(p => (activeMethod === 'simple' ? p.type === 'simple' : p.type === 'equation'))
        .map(pattern => pattern.name)
        .sort((a, b) => a.localeCompare(b)),
    [patterns, activeMethod],
  );

  useEffect(() => {
    if (!selectedPatternName) {
      return;
    }

    if (availablePatternNames.includes(selectedPatternName)) {
      return;
    }

    const currentPatternName =
      activeMethod === 'simple' ? simplePatternState.name : equationPatternState.name;

    // If the pattern name is still the same as the state, we don't need to clear it
    if (currentPatternName === selectedPatternName) {
      return;
    }

    setSelectedPatternName('');
    setOriginalPattern(null);
  }, [
    availablePatternNames,
    selectedPatternName,
    simplePatternState.name,
    equationPatternState.name,
    activeMethod,
  ]);

  const handleSimplePatternChange = (
    nextPattern: SimplePattern,
    action: SimpleRgbPatternAction,
  ) => {
    setSimplePatternState(nextPattern);

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

      setOriginalPattern(null);
      setSelectedPatternName('');
    }
  };

  const handleEquationPatternChange = (
    nextPattern: EquationPattern,
    action: EquationRgbPatternAction,
  ) => {
    setEquationPatternState(nextPattern);

    const result = equationPatternSchema.safeParse(nextPattern);
    if (!result.success) {
      setValidationErrors(result.error.issues.map(issue => issue.message));
    } else {
      setValidationErrors([]);
    }

    if (action.type === 'rename-pattern') {
      if (selectedPatternName === action.name) {
        return;
      }
      setOriginalPattern(null);
      setSelectedPatternName('');
    }
  };

  const handleMethodChange = (method: 'simple' | 'equation') => {
    setActiveMethod(method);

    const targetState = method === 'simple' ? simplePatternState : equationPatternState;

    const result =
      method === 'simple'
        ? simplePatternSchema.safeParse(targetState)
        : equationPatternSchema.safeParse(targetState);

    if (!result.success) {
      setValidationErrors(result.error.issues.map(issue => issue.message));
    } else {
      setValidationErrors([]);
    }

    const exists = patterns.some(p => p.type === method && p.name === targetState.name);

    if (exists) {
      setSelectedPatternName(targetState.name);
      const stored = getPattern(targetState.name);
      setOriginalPattern(stored ?? null);
    } else {
      setSelectedPatternName('');
      setOriginalPattern(null);
    }
  };

  const handlePatternSelect = (event: ChangeEvent<HTMLSelectElement>) => {
    const nextName = event.target.value;

    if (nextName === '') {
      setSelectedPatternName('');
      setOriginalPattern(null);
      if (activeMethod === 'simple') {
        const newPattern = createEmptyPattern();
        setSimplePatternState(newPattern);
        const result = simplePatternSchema.safeParse(newPattern);
        setValidationErrors(result.success ? [] : result.error.issues.map(issue => issue.message));
      } else {
        const newPattern = createDefaultEquationPattern();
        setEquationPatternState(newPattern);
        const result = equationPatternSchema.safeParse(newPattern);
        setValidationErrors(result.success ? [] : result.error.issues.map(issue => issue.message));
      }
      return;
    }

    setSelectedPatternName(nextName);
    const stored = getPattern(nextName);
    if (stored) {
      setOriginalPattern(stored);
      if (activeMethod === 'simple' && stored.type === 'simple') {
        setSimplePatternState(stored);
        const result = simplePatternSchema.safeParse(stored);
        if (!result.success) {
          setValidationErrors(result.error.issues.map(issue => issue.message));
        } else {
          setValidationErrors([]);
        }
      } else if (activeMethod === 'equation' && stored.type === 'equation') {
        setEquationPatternState(stored);
        const result = equationPatternSchema.safeParse(stored);
        if (!result.success) {
          setValidationErrors(result.error.issues.map(issue => issue.message));
        } else {
          setValidationErrors([]);
        }
      }
    }
  };

  const handlePatternSave = () => {
    setValidationErrors([]);
    const pattern = activeMethod === 'simple' ? simplePatternState : equationPatternState;

    const result =
      activeMethod === 'simple'
        ? simplePatternSchema.safeParse(pattern)
        : equationPatternSchema.safeParse(pattern);

    if (!result.success) {
      setValidationErrors(result.error.issues.map(issue => issue.message));
      return;
    }

    const patternName = pattern.name.trim();
    const isOverwrite = availablePatternNames.includes(patternName);
    if (isOverwrite) {
      const shouldOverwrite = window.confirm(
        t('rgbPattern.simple.storage.overwriteConfirm', { name: patternName }),
      );
      if (!shouldOverwrite) {
        return;
      }
    }

    savePattern(pattern);
    setOriginalPattern(pattern);
    setSelectedPatternName(patternName);
  };

  const handlePatternDelete = () => {
    if (!selectedPatternName) {
      return;
    }

    const shouldDelete = window.confirm(
      t('rgbPattern.simple.storage.deleteConfirm', { name: selectedPatternName }),
    );

    if (!shouldDelete) {
      return;
    }

    setOriginalPattern(null);
    deletePattern(selectedPatternName);
    setSelectedPatternName('');
    if (activeMethod === 'simple') {
      const newPattern = createEmptyPattern();
      setSimplePatternState(newPattern);
      const result = simplePatternSchema.safeParse(newPattern);
      setValidationErrors(result.success ? [] : result.error.issues.map(issue => issue.message));
    } else {
      const newPattern = createDefaultEquationPattern();
      setEquationPatternState(newPattern);
      const result = equationPatternSchema.safeParse(newPattern);
      setValidationErrors(result.success ? [] : result.error.issues.map(issue => issue.message));
    }
  };

  const isDirty = useMemo(() => {
    if (!selectedPatternName || !originalPattern) {
      return true;
    }

    const currentPattern = activeMethod === 'simple' ? simplePatternState : equationPatternState;
    return JSON.stringify(currentPattern) !== JSON.stringify(originalPattern);
  }, [
    selectedPatternName,
    originalPattern,
    activeMethod,
    simplePatternState,
    equationPatternState,
  ]);

  const isSimpleMethod = activeMethod === 'simple';

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('rgbPattern.title')}</h2>
        <p className="theme-muted">{t('rgbPattern.subtitle')}</p>
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
              handleMethodChange('simple');
            }}
            type="button"
          >
            {t('rgbPattern.methodSwitcher.simple')}
          </button>
          <button
            className={`px-4 py-2 text-sm transition-colors ${
              !isSimpleMethod
                ? 'bg-[rgb(var(--accent)/0.25)] text-[rgb(var(--accent-contrast)/1)]'
                : 'theme-muted hover:bg-[rgb(var(--surface-raised)/0.5)]'
            }`}
            onClick={() => {
              handleMethodChange('equation');
            }}
            type="button"
          >
            {t('rgbPattern.methodSwitcher.equation')}
          </button>
        </div>
      </div>

      <article
        className={`space-y-6 rounded-2xl border border-dashed border-white/10 bg-[rgb(var(--surface-raised)/0.35)] p-6`}
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
          <div className="flex flex-col gap-2 md:flex-row md:items-end md:justify-between">
            <label className="flex w-full max-w-sm flex-col gap-2 text-sm">
              <span className="font-medium">{t('rgbPattern.simple.storage.selectLabel')}</span>
              <select
                className="rounded-xl border border-solid theme-border bg-transparent px-3 py-2"
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
            <div className="flex flex-wrap items-center gap-2">
              <button
                className="rounded-full border border-solid border-red-400 px-4 py-2 text-sm font-medium text-red-100 transition-transform hover:scale-[1.01] disabled:opacity-50"
                onClick={handlePatternDelete}
                type="button"
                disabled={!selectedPatternName}
              >
                {t('rgbPattern.simple.storage.deleteButton')}
              </button>
              <button
                className="rounded-full bg-[rgb(var(--accent)/1)] px-4 py-2 text-sm font-medium text-[rgb(var(--surface-contrast)/1)] transition-transform hover:scale-[1.01] disabled:opacity-50"
                onClick={handlePatternSave}
                type="button"
                disabled={!isDirty || validationErrors.length > 0}
              >
                {t('rgbPattern.simple.storage.saveButton')}
              </button>
            </div>
          </div>
        </header>

        {validationErrors.length > 0 && (
          <div className="rounded-xl border border-red-500/50 bg-red-500/10 p-4 text-sm text-red-200">
            <ul className="list-inside list-disc space-y-1">
              {validationErrors.map((error, index) => (
                <li key={index}>{error}</li>
              ))}
            </ul>
          </div>
        )}

        {isSimpleMethod ? (
          <SimpleRgbPatternPanel onChange={handleSimplePatternChange} value={simplePatternState} />
        ) : (
          <EquationRgbPatternPanel
            onChange={handleEquationPatternChange}
            pattern={equationPatternState}
          />
        )}
      </article>
    </section>
  );
};
