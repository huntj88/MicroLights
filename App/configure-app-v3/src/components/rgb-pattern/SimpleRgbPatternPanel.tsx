import type { ChangeEvent } from 'react';
import { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { hexColorSchema, type SimplePattern } from '../../app/models/mode';

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
  value: SimplePattern;
  onChange: (state: SimplePattern, action: SimpleRgbPatternAction) => void;
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

const convertPatternToSteps = (pattern: SimplePattern): SimpleRgbPatternStep[] => {
  if (!pattern.changeAt.length) {
    return [];
  }

  const steps: SimpleRgbPatternStep[] = [];

  for (let index = 0; index < pattern.changeAt.length; index += 1) {
    const current = pattern.changeAt[index];
    const endMs =
      index === pattern.changeAt.length - 1 ? pattern.duration : pattern.changeAt[index + 1].ms;
    const duration = Math.max(endMs - current.ms, 0);

    steps.push({
      id: `${STEP_ID_PREFIX}${current.ms.toString()}-${index.toString()}`,
      color: hexColorSchema.parse(current.output),
      durationMs: duration,
    });
  }

  return steps;
};

const createPatternFromSteps = (
  pattern: SimplePattern,
  steps: SimpleRgbPatternStep[],
): SimplePattern => {
  let cursor = 0;

  const changeAt = steps.map(step => {
    const entry = {
      ms: cursor,
      output: hexColorSchema.parse(step.color),
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
  const [selectedStepIndex, setSelectedStepIndex] = useState<number | null>(null);

  useEffect(() => {
    setStepDurationDrafts(previous => {
      const nextEntries = steps.map(
        step => [step.id, previous[step.id] ?? String(step.durationMs)] as const,
      );
      const shouldUpdate =
        nextEntries.length !== Object.keys(previous).length ||
        nextEntries.some(([id, value]) => previous[id] !== value);

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
  const defaultModalDuration =
    steps.length > 0 ? String(steps[lastStepIndex].durationMs) : DEFAULT_DURATION_MS;

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
  const canConfirmModal =
    modalDurationMs !== '' && Number.isFinite(parsedModalDuration) && parsedModalDuration > 0;

  const totalDuration = useMemo(
    () => steps.reduce((accumulator, step) => accumulator + step.durationMs, 0),
    [steps],
  );

  const emitChange = (nextSteps: SimpleRgbPatternStep[], action: SimpleRgbPatternAction) => {
    const nextPattern = createPatternFromSteps(value, nextSteps);
    onChange(nextPattern, action);
  };

  const handleStepUpdate = (stepId: string, updates: Partial<SimpleRgbPatternStep>) => {
    const nextSteps = steps.map(step => (step.id === stepId ? { ...step, ...updates } : step));
    const updatedStep = nextSteps.find(step => step.id === stepId);
    if (!updatedStep) {
      return;
    }

    emitChange(nextSteps, { type: 'update-step', stepId, step: updatedStep });
  };

  const handleNameChange = (event: ChangeEvent<HTMLInputElement>) => {
    const nextName = event.target.value;
    const nextPattern: SimplePattern = {
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

    setStepDurationDrafts(previous => ({
      ...previous,
      [stepId]: duration,
    }));

    if (duration === '') {
      handleStepUpdate(stepId, { durationMs: Number.NaN });
      return;
    }

    const parsed = Number.parseInt(duration, 10);
    if (!Number.isFinite(parsed) || parsed < 0) {
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
    setSelectedStepIndex(steps.length);
  };

  const handleRemove = (stepId: string) => {
    const indexToRemove = steps.findIndex(step => step.id === stepId);
    const nextSteps = steps.filter(step => step.id !== stepId);
    emitChange(nextSteps, { type: 'remove-step', stepId });

    if (selectedStepIndex === indexToRemove) {
      setSelectedStepIndex(null);
    } else if (selectedStepIndex !== null && indexToRemove < selectedStepIndex) {
      setSelectedStepIndex(selectedStepIndex - 1);
    }
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

    if (selectedStepIndex === fromIndex) {
      setSelectedStepIndex(toIndex);
    } else if (selectedStepIndex === toIndex) {
      setSelectedStepIndex(fromIndex);
    }
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

    if (selectedStepIndex !== null && sourceIndex <= selectedStepIndex) {
      setSelectedStepIndex(selectedStepIndex + 1);
    }
  };

  const patternSegments = useMemo(() => {
    if (steps.length === 0) {
      return null;
    }

    return steps.map((step, index) => {
      const isSelected = index === selectedStepIndex;
      return (
        <button
          key={step.id}
          aria-label={t('rgbPattern.simple.preview.segmentLabel', {
            color: step.color,
            duration: step.durationMs,
          })}
          aria-pressed={isSelected}
          className={`flex flex-1 items-center justify-center text-xs font-medium text-[rgb(var(--surface-contrast)/1)] transition-all hover:opacity-90 focus:outline-none focus:ring-2 focus:ring-[rgb(var(--accent)/1)] focus:ring-inset ${
            isSelected
              ? 'z-10 scale-[1.02] shadow-lg ring-2 ring-[rgb(var(--accent)/1)] ring-inset'
              : ''
          }`}
          onClick={() => {
            setSelectedStepIndex(index);
          }}
          style={{
            backgroundColor: step.color,
            flexGrow: totalDuration > 0 ? step.durationMs : 1,
          }}
          title={t('rgbPattern.simple.preview.segmentLabel', {
            color: step.color,
            duration: step.durationMs,
          })}
          type="button"
        >
          <span className="px-2 py-1 mix-blend-difference">{step.durationMs}ms</span>
        </button>
      );
    });
  }, [steps, t, totalDuration, selectedStepIndex]);

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
          {steps.length === 0 ? (
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

      {selectedStepIndex !== null && steps[selectedStepIndex] && (
        <section className="theme-panel theme-border mt-4 space-y-4 rounded-xl border p-4 animate-in fade-in slide-in-from-top-2">
          <header className="flex items-center justify-between">
            <h3 className="text-lg font-semibold">
              {t('rgbPattern.simple.steps.title')} #{selectedStepIndex + 1}
            </h3>
            <button
              aria-label={t('rgbPattern.simple.steps.closeEditor')}
              className="rounded-full p-2 text-xl transition-colors hover:bg-[rgb(var(--surface-raised)/1)]"
              onClick={() => {
                setSelectedStepIndex(null);
              }}
              type="button"
            >
              ✕
            </button>
          </header>

          <div className="flex flex-wrap items-center gap-6">
            <label className="flex items-center gap-3 text-sm font-medium">
              <span>{t('rgbPattern.simple.form.colorLabel')}</span>
              <input
                className="h-10 w-10 rounded-full border border-solid theme-border"
                onChange={event => {
                  handleStepColorChange(steps[selectedStepIndex].id, event.target.value);
                }}
                type="color"
                value={steps[selectedStepIndex].color}
              />
              <span className="font-mono uppercase">{steps[selectedStepIndex].color}</span>
            </label>

            <label className="flex items-center gap-3 text-sm font-medium">
              <span>{t('rgbPattern.simple.form.durationLabel')}</span>
              <div className="relative">
                <input
                  className="w-24 rounded-lg border border-solid theme-border bg-transparent px-3 py-2 pr-8 text-sm"
                  inputMode="numeric"
                  min={1}
                  onChange={event => {
                    handleStepDurationChange(steps[selectedStepIndex].id, event.target.value);
                  }}
                  step={1}
                  type="number"
                  value={
                    stepDurationDrafts[steps[selectedStepIndex].id] ??
                    String(steps[selectedStepIndex].durationMs)
                  }
                />
                <span className="pointer-events-none absolute right-3 top-1/2 -translate-y-1/2 text-xs theme-muted">
                  ms
                </span>
              </div>
            </label>
          </div>

          <div className="flex flex-wrap gap-2 pt-2">
            <button
              className="rounded-full border border-solid theme-border px-4 py-2 text-sm font-medium transition-colors hover:bg-[rgb(var(--surface-raised)/1)] disabled:opacity-40"
              disabled={selectedStepIndex === 0}
              onClick={() => {
                handleMove(steps[selectedStepIndex].id, 'up');
              }}
              type="button"
            >
              ← {t('rgbPattern.simple.steps.moveUp')}
            </button>
            <button
              className="rounded-full border border-solid theme-border px-4 py-2 text-sm font-medium transition-colors hover:bg-[rgb(var(--surface-raised)/1)] disabled:opacity-40"
              disabled={selectedStepIndex === steps.length - 1}
              onClick={() => {
                handleMove(steps[selectedStepIndex].id, 'down');
              }}
              type="button"
            >
              {t('rgbPattern.simple.steps.moveDown')} →
            </button>
            <div className="flex-1" />
            <button
              className="rounded-full border border-solid theme-border px-4 py-2 text-sm font-medium transition-colors hover:bg-[rgb(var(--surface-raised)/1)]"
              onClick={() => {
                handleDuplicate(steps[selectedStepIndex].id);
              }}
              type="button"
            >
              {t('rgbPattern.simple.steps.duplicate')}
            </button>
            <button
              className="rounded-full border border-solid theme-border bg-red-500/10 px-4 py-2 text-sm font-medium text-red-500 transition-colors hover:bg-red-500/20"
              onClick={() => {
                handleRemove(steps[selectedStepIndex].id);
              }}
              type="button"
            >
              {t('rgbPattern.simple.steps.remove')}
            </button>
          </div>
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
