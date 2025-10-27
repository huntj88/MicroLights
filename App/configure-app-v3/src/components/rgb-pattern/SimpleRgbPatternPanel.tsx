import type { ChangeEvent } from 'react';
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
  | { type: 'rename-pattern'; name: string }
  | { type: 'update-step'; stepId: string; step: SimpleRgbPatternStep };

export interface SimpleRgbPatternPanelProps {
  value: ModePattern;
  onChange: (state: ModePattern, action: SimpleRgbPatternAction) => void;
}

const STEP_ID_PREFIX = 'rgb-step-';
const DEFAULT_COLOR = '#ff7b00';
const DEFAULT_DURATION_MS = '250';
const MAX_DURATION_LENGTH = 6;

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
  const [stepDurationDrafts, setStepDurationDrafts] = useState<Record<string, string>>({});
  const [isAddModalOpen, setIsAddModalOpen] = useState(false);
  const [modalColor, setModalColor] = useState(DEFAULT_COLOR);
  const [modalDurationMs, setModalDurationMs] = useState(DEFAULT_DURATION_MS);

  useEffect(() => {
    setStepDurationDrafts((previous) => {
      const nextEntries = steps.map((step) => [step.id, previous[step.id] ?? String(step.durationMs)] as const);
      const shouldUpdate =
        nextEntries.length !== Object.keys(previous).length
        || nextEntries.some(([id, value]) => previous[id] !== value);

      if (!shouldUpdate) {
        return previous;
      }

      const next: Record<string, string> = {};
      for (const [id, value] of nextEntries) {
        next[id] = value;
      }
      return next;
    });
  }, [steps]);

  const lastStepIndex = steps.length - 1;
  const defaultModalColor = steps.length > 0 ? steps[lastStepIndex].color : DEFAULT_COLOR;
  const defaultModalDuration = steps.length > 0
    ? String(steps[lastStepIndex].durationMs)
    : DEFAULT_DURATION_MS;

  useEffect(() => {
    if (isAddModalOpen) {
      return;
    }

    setModalColor(defaultModalColor);
    setModalDurationMs(defaultModalDuration);
  }, [defaultModalColor, defaultModalDuration, isAddModalOpen]);

  useEffect(() => {
    if (!isAddModalOpen) {
      return;
    }

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key !== 'Escape') {
        return;
      }

      event.preventDefault();
      setIsAddModalOpen(false);
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, [isAddModalOpen]);

  const parsedModalDuration = Number.parseInt(modalDurationMs, 10);
  const canConfirmModal = modalDurationMs !== ''
    && Number.isFinite(parsedModalDuration)
    && parsedModalDuration > 0;

  const totalDuration = useMemo(
    () => steps.reduce((accumulator, step) => accumulator + step.durationMs, 0),
    [steps],
  );

  const emitChange = (nextSteps: SimpleRgbPatternStep[], action: SimpleRgbPatternAction) => {
    const nextPattern = createPatternFromSteps(value, nextSteps);
    onChange(nextPattern, action);
  };

  const handleStepUpdate = (stepId: string, updates: Partial<SimpleRgbPatternStep>) => {
    const nextSteps = steps.map((step) => (step.id === stepId ? { ...step, ...updates } : step));
    const updatedStep = nextSteps.find((step) => step.id === stepId);
    if (!updatedStep) {
      return;
    }

    emitChange(nextSteps, { type: 'update-step', stepId, step: updatedStep });
  };

  const handleNameChange = (event: ChangeEvent<HTMLInputElement>) => {
    const nextName = event.target.value;
    const nextPattern: ModePattern = {
      ...value,
      name: nextName,
    };

    onChange(nextPattern, { type: 'rename-pattern', name: nextName });
  };

  const handleStepColorChange = (stepId: string, color: string) => {
    handleStepUpdate(stepId, { color });
  };

  const handleStepDurationChange = (stepId: string, duration: string) => {
    if (duration.length > MAX_DURATION_LENGTH) {
      return;
    }

    if (!/^[0-9]*$/.test(duration)) {
      return;
    }

    setStepDurationDrafts((previous) => ({
      ...previous,
      [stepId]: duration,
    }));

    if (duration === '') {
      return;
    }

    const parsed = Number.parseInt(duration, 10);
    if (!Number.isFinite(parsed) || parsed <= 0) {
      return;
    }

    handleStepUpdate(stepId, { durationMs: parsed });
  };

  const handleModalColorChange = (event: ChangeEvent<HTMLInputElement>) => {
    setModalColor(event.target.value);
  };

  const handleModalDurationChange = (event: ChangeEvent<HTMLInputElement>) => {
    const duration = event.target.value;
    if (duration.length > MAX_DURATION_LENGTH) {
      return;
    }

    if (duration === '') {
      setModalDurationMs(duration);
      return;
    }

    if (!/^[0-9]+$/.test(duration)) {
      return;
    }

    setModalDurationMs(duration);
  };

  const openAddModal = () => {
    setIsAddModalOpen(true);
  };

  const handleModalCancel = () => {
    setIsAddModalOpen(false);
  };

  const handleModalConfirm = () => {
    if (!canConfirmModal) {
      return;
    }

    const step = createStep(modalColor, parsedModalDuration);
    emitChange([...steps, step], { type: 'add-step', step });
    setIsAddModalOpen(false);
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
      <section aria-label={t('rgbPattern.simple.form.ariaLabel')} className="space-y-4">
        <label className="flex flex-col gap-2 text-sm">
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
      </section>

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
          <button
            aria-label={t('rgbPattern.simple.form.addButton')}
            className="flex min-w-[56px] items-center justify-center border-l border-white/10 bg-[rgb(var(--surface-raised)/0.4)] text-xl font-semibold text-[rgb(var(--accent)/1)] transition hover:bg-[rgb(var(--surface-raised)/0.6)]"
            onClick={openAddModal}
            title={t('rgbPattern.simple.form.addButton')}
            type="button"
          >
            <span aria-hidden="true">＋</span>
          </button>
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
                  className="theme-panel theme-border flex flex-wrap items-center justify-between gap-4 rounded-xl border px-4 py-3"
                >
                  <div className="flex flex-1 flex-wrap items-center gap-4">
                    <label className="flex items-center gap-3 text-sm font-medium">
                      <span className="sr-only">
                        {t('rgbPattern.simple.steps.colorEditLabel', { index: index + 1 })}
                      </span>
                      <input
                        aria-label={t('rgbPattern.simple.steps.colorEditLabel', { index: index + 1 })}
                        className="h-10 w-10 rounded-full border border-solid theme-border"
                        onChange={(event) => {
                          handleStepColorChange(step.id, event.target.value);
                        }}
                        type="color"
                        value={step.color}
                      />
                      <span className="flex items-center gap-2">
                        {t('rgbPattern.simple.steps.entryLabel', {
                          color: step.color.toUpperCase(),
                          duration: step.durationMs,
                        })}
                      </span>
                    </label>
                    <label className="flex items-center gap-2 text-xs font-medium">
                      <span>{t('rgbPattern.simple.steps.durationEditLabel', { index: index + 1 })}</span>
                      <input
                        aria-label={t('rgbPattern.simple.steps.durationEditLabel', { index: index + 1 })}
                        className="w-24 rounded-lg border border-solid theme-border bg-transparent px-2 py-1 text-sm"
                        inputMode="numeric"
                        min={1}
                        onChange={(event) => {
                          handleStepDurationChange(step.id, event.target.value);
                        }}
                        step={1}
                        type="number"
                        value={stepDurationDrafts[step.id] ?? String(step.durationMs)}
                      />
                    </label>
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

      {isAddModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 px-4">
          <div
            aria-modal="true"
            className="w-full max-w-sm space-y-4 rounded-2xl border border-white/20 bg-[rgb(var(--surface-raised)/0.95)] p-6 shadow-xl"
            role="dialog"
          >
            <header className="space-y-1">
              <h4 className="text-lg font-semibold">{t('rgbPattern.simple.addModal.title')}</h4>
              <p className="theme-muted text-sm">
                {t('rgbPattern.simple.preview.summary', {
                  total: totalDuration + (canConfirmModal ? parsedModalDuration : 0),
                })}
              </p>
            </header>
            <div className="space-y-4">
              <label className="flex flex-col gap-2 text-sm">
                <span className="font-medium">{t('rgbPattern.simple.form.colorLabel')}</span>
                <input
                  aria-describedby="simple-rgb-modal-color-helper"
                  className="h-12 w-24 rounded-full border border-solid theme-border"
                  onChange={handleModalColorChange}
                  type="color"
                  value={modalColor}
                />
                <span className="theme-muted text-xs" id="simple-rgb-modal-color-helper">
                  {t('rgbPattern.simple.form.colorHelper')}
                </span>
              </label>
              <label className="flex flex-col gap-2 text-sm">
                <span className="font-medium">{t('rgbPattern.simple.form.durationLabel')}</span>
                <input
                  aria-describedby="simple-rgb-modal-duration-helper"
                  className="w-full rounded-xl border border-solid theme-border bg-transparent px-3 py-2"
                  inputMode="numeric"
                  min={1}
                  onChange={handleModalDurationChange}
                  step={1}
                  type="number"
                  value={modalDurationMs}
                />
                <span className="theme-muted text-xs" id="simple-rgb-modal-duration-helper">
                  {t('rgbPattern.simple.form.durationHelper')}
                </span>
              </label>
            </div>
            <footer className="flex justify-end gap-3">
              <button
                className="rounded-full border border-solid theme-border px-4 py-2 text-sm font-medium transition hover:bg-[rgb(var(--surface-raised)/0.6)]"
                onClick={handleModalCancel}
                type="button"
              >
                {t('rgbPattern.simple.addModal.cancel')}
              </button>
              <button
                className="rounded-full bg-[rgb(var(--accent)/1)] px-4 py-2 text-sm font-medium text-[rgb(var(--surface-contrast)/1)] transition hover:scale-[1.01] disabled:opacity-50"
                disabled={!canConfirmModal}
                onClick={handleModalConfirm}
                type="button"
              >
                {t('rgbPattern.simple.addModal.confirm')}
              </button>
            </footer>
          </div>
        </div>
      )}
    </div>
  );
};
