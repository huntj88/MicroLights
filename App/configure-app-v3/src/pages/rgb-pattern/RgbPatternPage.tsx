import type { ChangeEvent } from 'react';
import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import type { ModePattern } from '../../app/models/mode';
import { usePatternStore } from '../../app/providers/pattern-store';
import { EquationRgbPatternPanel } from '../../components/rgb-pattern/equation/EquationRgbPatternPanel';
import {
  type SimpleRgbPatternAction,
  SimpleRgbPatternPanel,
} from '../../components/rgb-pattern/SimpleRgbPatternPanel';

const createEmptyPattern = (): ModePattern => ({
  name: 'Simple RGB Pattern',
  duration: 0,
  changeAt: [],
});

export const RgbPatternPage = () => {
  const { t } = useTranslation();
  const [activeMethod, setActiveMethod] = useState<'simple' | 'equation'>('simple');
  const [selectedPatternName, setSelectedPatternName] = useState('');
  const [simplePatternState, setSimplePatternState] = useState<ModePattern>(createEmptyPattern);
  const patterns = usePatternStore(state => state.patterns);
  const savePattern = usePatternStore(state => state.savePattern);
  const deletePattern = usePatternStore(state => state.deletePattern);
  const getPattern = usePatternStore(state => state.getPattern);

  const availablePatternNames = useMemo(
    () => patterns.map(pattern => pattern.name).sort((a, b) => a.localeCompare(b)),
    [patterns],
  );

  useEffect(() => {
    if (!selectedPatternName) {
      return;
    }

    if (availablePatternNames.includes(selectedPatternName)) {
      return;
    }

    if (simplePatternState.name === selectedPatternName) {
      return;
    }

    setSelectedPatternName('');
  }, [availablePatternNames, selectedPatternName, simplePatternState.name]);

  const handleSimplePatternChange = (nextPattern: ModePattern, action: SimpleRgbPatternAction) => {
    setSimplePatternState(nextPattern);

    if (action.type === 'rename-pattern') {
      if (selectedPatternName === action.name) {
        return;
      }

      setSelectedPatternName('');
    }
  };

  const handleMethodChange = (method: 'simple' | 'equation') => {
    setActiveMethod(method);
  };

  const handlePatternSelect = (event: ChangeEvent<HTMLSelectElement>) => {
    const nextName = event.target.value;

    if (nextName === '') {
      setSelectedPatternName('');
      setSimplePatternState(createEmptyPattern());
      return;
    }

    setSelectedPatternName(nextName);
    const stored = getPattern(nextName);
    if (stored) {
      setSimplePatternState(stored);
    }
  };

  const handleSimplePatternSave = () => {
    const patternName = simplePatternState.name.trim();
    if (!patternName || simplePatternState.changeAt.length === 0) {
      return;
    }

    const isOverwrite = availablePatternNames.includes(patternName);
    if (isOverwrite) {
      const shouldOverwrite = window.confirm(
        t('rgbPattern.simple.storage.overwriteConfirm', { name: patternName }),
      );
      if (!shouldOverwrite) {
        return;
      }
    }

    savePattern(simplePatternState);
    setSelectedPatternName(patternName);
  };

  const handleSimplePatternDelete = () => {
    if (!selectedPatternName) {
      return;
    }

    const shouldDelete = window.confirm(
      t('rgbPattern.simple.storage.deleteConfirm', { name: selectedPatternName }),
    );

    if (!shouldDelete) {
      return;
    }

    deletePattern(selectedPatternName);
    setSelectedPatternName('');
    setSimplePatternState(createEmptyPattern());
  };

  const isSimpleMethod = activeMethod === 'simple';
  const hasSimpleSteps = simplePatternState.changeAt.length > 0;

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
      {isSimpleMethod ? (
        <article className="space-y-6 rounded-2xl border border-dashed border-white/10 bg-[rgb(var(--surface-raised)/0.35)] p-6">
          <header className="space-y-3">
            <div className="space-y-1">
              <h3 className="text-2xl font-semibold">{t('rgbPattern.simple.title')}</h3>
              <p className="theme-muted text-sm">{t('rgbPattern.simple.description')}</p>
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
                  onClick={handleSimplePatternDelete}
                  type="button"
                  disabled={!selectedPatternName}
                >
                  {t('rgbPattern.simple.storage.deleteButton')}
                </button>
                <button
                  className="rounded-full bg-[rgb(var(--accent)/1)] px-4 py-2 text-sm font-medium text-[rgb(var(--surface-contrast)/1)] transition-transform hover:scale-[1.01] disabled:opacity-50"
                  onClick={handleSimplePatternSave}
                  type="button"
                  disabled={!simplePatternState.name.trim() || !hasSimpleSteps}
                >
                  {t('rgbPattern.simple.storage.saveButton')}
                </button>
              </div>
            </div>
          </header>
          <SimpleRgbPatternPanel
            onChange={handleSimplePatternChange}
            value={simplePatternState}
          />
        </article>
      ) : (
        <article className="theme-panel theme-border rounded-2xl border p-6">
          <EquationRgbPatternPanel />
        </article>
      )}
    </section>
  );
};
