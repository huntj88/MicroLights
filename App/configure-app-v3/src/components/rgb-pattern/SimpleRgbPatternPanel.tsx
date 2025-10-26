import type { ChangeEvent, FormEvent } from 'react';
import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import type { ModePattern } from '../../app/models/mode';

export interface SimpleRgbPatternStep {
  id: string;
  color: string;
  durationMs: number;
}

export type SimpleRgbPatternAction =
  | { type: 'add-step'; step: SimpleRgbPatternStep }
  | { type: 'remove-step'; stepId: string }
  | { type: 'move-step'; fromIndex: number; toIndex: number }
  | { type: 'duplicate-step'; sourceId: string; newStep: SimpleRgbPatternStep }
  | { type: 'rename-pattern'; name: string };

export interface SimpleRgbPatternPanelProps {
  value: ModePattern;
  onChange: (state: ModePattern, action: SimpleRgbPatternAction) => void;
}

const STEP_ID_PREFIX = 'rgb-step-';
const DEFAULT_DURATION_MS = '250';

const createStep = (color: string, durationMs: number): SimpleRgbPatternStep => ({
  id: `${STEP_ID_PREFIX}${Date.now().toString(36)}-${Math.random().toString(16).slice(2)}`,
  color,
  durationMs,
});

const convertPatternToSteps = (pattern: ModePattern): SimpleRgbPatternStep[] => {
  if (!pattern.changeAt.length || pattern.duration <= 0) {
    return [];
  }

  const steps: SimpleRgbPatternStep[] = [];

  for (let index = 0; index < pattern.changeAt.length; index += 1) {
    const current = pattern.changeAt[index];
    const endMs = index === pattern.changeAt.length - 1
      ? pattern.duration
      : pattern.changeAt[index + 1].ms;
    const duration = Math.max(endMs - current.ms, 0);

    if (duration === 0) {
      continue;
    }

    steps.push({
      id: `${STEP_ID_PREFIX}${current.ms.toString()}-${index.toString()}`,
      color: current.output as string,
      durationMs: duration,
    });
  }

  return steps;
};

const createPatternFromSteps = (pattern: ModePattern, steps: SimpleRgbPatternStep[]): ModePattern => {
  let cursor = 0;

  const changeAt = steps.map(step => {
    const entry = {
      ms: cursor,
      output: step.color as ModePattern['changeAt'][number]['output'],
    };
    cursor += step.durationMs;
    return entry;
  });

  return {
    ...pattern,
    duration: cursor,
    changeAt,
  };
};

export const SimpleRgbPatternPanel = ({ value, onChange }: SimpleRgbPatternPanelProps) => {
  const { t } = useTranslation();

  const steps = useMemo(() => convertPatternToSteps(value), [value]);
  const [draftColor, setDraftColor] = useState(() => steps[steps.length - 1]?.color ?? '#ff7b00');
  const [draftDurationMs, setDraftDurationMs] = useState(DEFAULT_DURATION_MS);

  useEffect(() => {
    if (steps.length === 0) {
      setDraftColor('#ff7b00');
      setDraftDurationMs(DEFAULT_DURATION_MS);
      return;
    }

    setDraftColor(steps[steps.length - 1].color);
    setDraftDurationMs(String(steps[steps.length - 1].durationMs));
  }, [steps]);

  const parsedDuration = Number.parseInt(draftDurationMs, 10);
  const canAddStep = draftDurationMs !== '' && Number.isFinite(parsedDuration) && parsedDuration > 0;

  const totalDuration = useMemo(
    () => steps.reduce((accumulator, step) => accumulator + step.durationMs, 0),
    [steps],
  );

  const emitChange = (nextSteps: SimpleRgbPatternStep[], action: SimpleRgbPatternAction) => {
    const nextPattern = createPatternFromSteps(value, nextSteps);
    onChange(nextPattern, action);
  };

  const handleNameChange = (event: ChangeEvent<HTMLInputElement>) => {
    const nextName = event.target.value;
    const nextPattern: ModePattern = {
      ...value,
      name: nextName,
    };

    onChange(nextPattern, { type: 'rename-pattern', name: nextName });
  };

  const handleColorChange = (event: ChangeEvent<HTMLInputElement>) => {
    setDraftColor(event.target.value);
  };

  const handleDurationChange = (event: ChangeEvent<HTMLInputElement>) => {
    const duration = event.target.value;
    if (duration.length > 6) {
      return;
    }

    if (duration === '') {
      setDraftDurationMs(duration);
      return;
    }

    if (!/^[0-9]+$/.test(duration)) {
      return;
    }

    setDraftDurationMs(duration);
  };

  const handleAddStep = (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();

    if (!canAddStep) {
      return;
    }

    const step = createStep(draftColor, parsedDuration);
    emitChange([...steps, step], { type: 'add-step', step });
    setDraftDurationMs(DEFAULT_DURATION_MS);
  };

  const handleRemove = (stepId: string) => {
    const nextSteps = steps.filter(step => step.id !== stepId);
    emitChange(nextSteps, { type: 'remove-step', stepId });
  };

  const handleMove = (stepId: string, direction: 'up' | 'down') => {
    const fromIndex = steps.findIndex(step => step.id === stepId);
    if (fromIndex === -1) {
      return;
    }

    const toIndex = direction === 'up' ? fromIndex - 1 : fromIndex + 1;
    if (toIndex < 0 || toIndex >= steps.length) {
      return;
    }

    const nextSteps = [...steps];
    const [movingStep] = nextSteps.splice(fromIndex, 1);
    nextSteps.splice(toIndex, 0, movingStep);

    emitChange(nextSteps, { type: 'move-step', fromIndex, toIndex });
  };

  const handleDuplicate = (stepId: string) => {
    const sourceIndex = steps.findIndex(step => step.id === stepId);
    if (sourceIndex === -1) {
      return;
    }

    const sourceStep = steps[sourceIndex];
    const duplicate = createStep(sourceStep.color, sourceStep.durationMs);

    const nextSteps = [...steps];
    nextSteps.splice(sourceIndex + 1, 0, duplicate);

    emitChange(nextSteps, { type: 'duplicate-step', sourceId: stepId, newStep: duplicate });
  };

  const patternSegments = useMemo(() => {
    if (totalDuration === 0) {
      return null;
    }

    return steps.map(step => (
      <div
        key={step.id}
        aria-label={t('rgbPattern.simple.preview.segmentLabel', {
          color: step.color,
          duration: step.durationMs,
        })}
        className="flex flex-1 items-center justify-center text-xs font-medium text-[rgb(var(--surface-contrast)/1)]"
        style={{
          backgroundColor: step.color,
          flexGrow: step.durationMs,
        }}
        title={t('rgbPattern.simple.preview.segmentLabel', {
          color: step.color,
          duration: step.durationMs,
        })}
      >
        <span className="px-2 py-1 mix-blend-difference">{step.durationMs}ms</span>
      </div>
    ));
  }, [steps, t, totalDuration]);

  return (
    <div className="space-y-6">
      <form
        className="grid gap-4 md:grid-cols-[minmax(0,1fr)_auto_auto_auto] md:items-end"
        onSubmit={handleAddStep}
        aria-label={t('rgbPattern.simple.form.ariaLabel')}
      >
        <label className="flex flex-col gap-2 text-sm md:col-span-4">
          <span className="font-medium">{t('rgbPattern.simple.form.nameLabel')}</span>
          <input
            aria-describedby="simple-rgb-name-helper"
            className="w-full rounded-xl border border-solid theme-border bg-transparent px-3 py-2"
            onChange={handleNameChange}
            placeholder={t('rgbPattern.simple.form.namePlaceholder')}
            type="text"
            value={value.name}
          />
          <span className="theme-muted text-xs" id="simple-rgb-name-helper">
            {t('rgbPattern.simple.form.nameHelper')}
          </span>
        </label>
        <label className="flex flex-col gap-2 text-sm">
          <span className="font-medium">{t('rgbPattern.simple.form.colorLabel')}</span>
          <input
            aria-describedby="simple-rgb-color-helper"
            className="h-12 w-24 rounded-full border border-solid theme-border"
            onChange={handleColorChange}
            type="color"
            value={draftColor}
          />
          <span className="theme-muted text-xs" id="simple-rgb-color-helper">
            {t('rgbPattern.simple.form.colorHelper')}
          </span>
        </label>

        <label className="flex flex-col gap-2 text-sm">
          <span className="font-medium">{t('rgbPattern.simple.form.durationLabel')}</span>
          <input
            aria-describedby="simple-rgb-duration-helper"
            className="w-full rounded-xl border border-solid theme-border bg-transparent px-3 py-2"
            inputMode="numeric"
            min={1}
            step={1}
            onChange={handleDurationChange}
            type="number"
            value={draftDurationMs}
          />
          <span className="theme-muted text-xs" id="simple-rgb-duration-helper">
            {t('rgbPattern.simple.form.durationHelper')}
          </span>
        </label>

        <div className="flex items-center md:justify-end">
          <button
            className="rounded-full bg-[rgb(var(--accent)/1)] px-4 py-2 text-sm font-medium text-[rgb(var(--surface-contrast)/1)] transition-transform hover:scale-[1.01] disabled:opacity-50"
            type="submit"
            disabled={!canAddStep}
          >
            {t('rgbPattern.simple.form.addButton')}
          </button>
        </div>
      </form>

      <section aria-live="polite" className="space-y-3">
        <header className="space-y-1">
          <h3 className="text-lg font-semibold">{t('rgbPattern.simple.preview.title')}</h3>
          <p className="theme-muted text-sm">
            {totalDuration === 0
              ? t('rgbPattern.simple.preview.empty')
              : t('rgbPattern.simple.preview.summary', { total: totalDuration })}
          </p>
        </header>

        <div className="theme-panel theme-border flex min-h-[56px] items-stretch overflow-hidden rounded-xl border">
          {totalDuration === 0 ? (
            <div className="flex flex-1 items-center justify-center text-sm">
              <span className="theme-muted">{t('rgbPattern.simple.preview.empty')}</span>
            </div>
          ) : (
            patternSegments
          )}
        </div>
      </section>

      {steps.length > 0 && (
        <section className="space-y-3">
          <header className="space-y-1">
            <h3 className="text-lg font-semibold">{t('rgbPattern.simple.steps.title')}</h3>
            <p className="theme-muted text-sm">{t('rgbPattern.simple.steps.subtitle')}</p>
          </header>
          <ol className="space-y-2" aria-live="polite">
            {steps.map((step, index) => {
              const isFirst = index === 0;
              const isLast = index === steps.length - 1;

              return (
                <li
                  key={step.id}
                  className="theme-panel theme-border flex flex-wrap items-center justify-between gap-3 rounded-xl border px-4 py-3"
                >
                  <div className="flex flex-col">
                    <span className="flex items-center gap-2 text-sm font-medium">
                      <span
                        aria-hidden="true"
                        className="h-4 w-4 rounded-full border border-white/40"
                        data-testid="rgb-step-color"
                        style={{ backgroundColor: step.color }}
                      />
                      {t('rgbPattern.simple.steps.entryLabel', {
                        color: step.color.toUpperCase(),
                        duration: step.durationMs,
                      })}
                    </span>
                    <span className="theme-muted text-xs">
                      {t('rgbPattern.simple.steps.duration', { duration: step.durationMs })}
                    </span>
                  </div>
                  <div className="flex flex-wrap items-center gap-2">
                    <button
                      aria-label={t('rgbPattern.simple.steps.moveUp')}
                      className="rounded-full border border-solid theme-border px-3 py-1 text-xs font-medium transition-colors hover:bg-[rgb(var(--surface-raised)/1)] disabled:opacity-40"
                      disabled={isFirst}
                      onClick={() => {
                        handleMove(step.id, 'up');
                      }}
                      type="button"
                    >
                      {t('rgbPattern.simple.steps.moveUp')}
                    </button>
                    <button
                      aria-label={t('rgbPattern.simple.steps.moveDown')}
                      className="rounded-full border border-solid theme-border px-3 py-1 text-xs font-medium transition-colors hover:bg-[rgb(var(--surface-raised)/1)] disabled:opacity-40"
                      disabled={isLast}
                      onClick={() => {
                        handleMove(step.id, 'down');
                      }}
                      type="button"
                    >
                      {t('rgbPattern.simple.steps.moveDown')}
                    </button>
                    <button
                      aria-label={t('rgbPattern.simple.steps.duplicate')}
                      className="rounded-full border border-solid theme-border px-3 py-1 text-xs font-medium transition-colors hover:bg-[rgb(var(--surface-raised)/1)]"
                      onClick={() => {
                        handleDuplicate(step.id);
                      }}
                      type="button"
                    >
                      {t('rgbPattern.simple.steps.duplicate')}
                    </button>
                    <button
                      className="rounded-full border border-solid theme-border px-3 py-1 text-xs font-medium transition-colors hover:bg-[rgb(var(--surface-raised)/1)]"
                      onClick={() => {
                        handleRemove(step.id);
                      }}
                      type="button"
                    >
                      {t('rgbPattern.simple.steps.remove')}
                    </button>
                  </div>
                </li>
              );
            })}
          </ol>
        </section>
      )}
    </div>
  );
};
