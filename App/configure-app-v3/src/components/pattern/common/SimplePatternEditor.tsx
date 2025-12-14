import { type ChangeEvent, type ReactNode, useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { z } from 'zod';

import { PatternNameEditor } from './PatternNameEditor';
import type { PatternChange, SimplePattern } from '../../../app/models/mode';
import { PanelContainer } from '../../common/PanelContainer';
import { Section } from '../../common/Section';
import { StyledButton } from '../../common/StyledButton';

export interface SimplePatternStep<T> {
  id: string;
  value: T;
  durationMs: number;
}

export type SimplePatternAction<T> =
  | { type: 'add-step'; step: SimplePatternStep<T> }
  | { type: 'remove-step'; stepId: string }
  | { type: 'move-step'; fromIndex: number; toIndex: number }
  | { type: 'duplicate-step'; sourceId: string; newStep: SimplePatternStep<T> }
  | { type: 'rename-pattern'; name: string }
  | { type: 'update-step'; stepId: string; step: SimplePatternStep<T> };

export interface SimplePatternEditorProps<T> {
  value: SimplePattern;
  onChange: (state: SimplePattern, action: SimplePatternAction<T>) => void;

  // Configuration
  valueSchema: z.ZodType<T>;
  defaultValue: T;
  idPrefix?: string;

  // UI Components
  renderInput: (props: { value: T; onChange: (value: T) => void; id?: string }) => ReactNode;
  renderPreview: (props: {
    value: T;
    durationMs: number;
    isSelected: boolean;
    onClick: () => void;
    totalDuration: number;
  }) => ReactNode;

  // Labels
  labels: {
    valueLabel: string;
    valueHelper?: string;
  };
}

const STEP_ID_PREFIX = 'step-';
const DEFAULT_DURATION_MS = '250';
const MAX_DURATION_LENGTH = 6;

const createStep = <T,>(value: T, durationMs: number, prefix: string): SimplePatternStep<T> => ({
  id: `${prefix}${Date.now().toString(36)}-${Math.random().toString(16).slice(2)}`,
  value,
  durationMs,
});

const convertPatternToSteps = <T,>(
  pattern: SimplePattern,
  schema: z.ZodType<T>,
  prefix: string,
): SimplePatternStep<T>[] => {
  if (!pattern.changeAt.length) {
    return [];
  }

  const steps: SimplePatternStep<T>[] = [];

  for (let index = 0; index < pattern.changeAt.length; index += 1) {
    const current = pattern.changeAt[index];
    const endMs =
      index === pattern.changeAt.length - 1 ? pattern.duration : pattern.changeAt[index + 1].ms;
    const duration = Math.max(endMs - current.ms, 0);

    steps.push({
      id: `${prefix}${current.ms.toString()}-${index.toString()}`,
      value: schema.parse(current.output),
      durationMs: duration,
    });
  }

  return steps;
};

const createPatternFromSteps = <T,>(
  pattern: SimplePattern,
  steps: SimplePatternStep<T>[],
): SimplePattern => {
  let cursor = 0;

  const changeAt = steps.map(step => {
    const entry = {
      ms: cursor,
      output: step.value as PatternChange['output'], // We assume T is compatible with SimplePattern output
    };
    cursor += Number.isNaN(step.durationMs) ? 0 : step.durationMs;
    return entry;
  });

  return {
    ...pattern,
    duration: cursor,
    changeAt,
  };
};

export const SimplePatternEditor = <T,>({
  value,
  onChange,
  valueSchema,
  defaultValue,
  idPrefix = STEP_ID_PREFIX,
  renderInput,
  renderPreview,
  labels,
}: SimplePatternEditorProps<T>) => {
  const { t } = useTranslation();

  const steps = useMemo(
    () => convertPatternToSteps(value, valueSchema, idPrefix),
    [value, valueSchema, idPrefix],
  );
  const [stepDurationDrafts, setStepDurationDrafts] = useState<Record<string, string>>({});
  const [isAddModalOpen, setIsAddModalOpen] = useState(false);
  const [modalValue, setModalValue] = useState<T>(defaultValue);
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
  const defaultModalValue = steps.length > 0 ? steps[lastStepIndex].value : defaultValue;
  const defaultModalDuration =
    steps.length > 0 ? String(steps[lastStepIndex].durationMs) : DEFAULT_DURATION_MS;

  useEffect(() => {
    if (isAddModalOpen) {
      return;
    }

    setModalValue(defaultModalValue);
    setModalDurationMs(defaultModalDuration);
  }, [defaultModalValue, defaultModalDuration, isAddModalOpen]);

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

  const emitChange = (nextSteps: SimplePatternStep<T>[], action: SimplePatternAction<T>) => {
    const nextPattern = createPatternFromSteps(value, nextSteps);
    onChange(nextPattern, action);
  };

  const handleStepUpdate = (stepId: string, updates: Partial<SimplePatternStep<T>>) => {
    const nextSteps = steps.map(step => (step.id === stepId ? { ...step, ...updates } : step));
    const updatedStep = nextSteps.find(step => step.id === stepId);
    if (!updatedStep) {
      return;
    }

    emitChange(nextSteps, { type: 'update-step', stepId, step: updatedStep });
  };

  const handleNameChange = (name: string) => {
    const nextPattern: SimplePattern = {
      ...value,
      name,
    };

    onChange(nextPattern, { type: 'rename-pattern', name });
  };

  const handleStepValueChange = (stepId: string, newValue: T) => {
    handleStepUpdate(stepId, { value: newValue });
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

  const handleModalValueChange = (newValue: T) => {
    setModalValue(newValue);
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

    const step = createStep(modalValue, parsedModalDuration, idPrefix);
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
    const duplicate = createStep(sourceStep.value, sourceStep.durationMs, idPrefix);

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
        <div key={step.id} style={{ display: 'contents' }}>
          {renderPreview({
            value: step.value,
            durationMs: step.durationMs,
            isSelected,
            onClick: () => {
              setSelectedStepIndex(index);
            },
            totalDuration,
          })}
        </div>
      );
    });
  }, [steps, totalDuration, selectedStepIndex, renderPreview]);

  return (
    <PanelContainer>
      <PatternNameEditor name={value.name} onChange={handleNameChange} />

      <Section title={t('patternEditor.preview.title')}>
        <p className="theme-muted text-sm mb-2">
          {totalDuration === 0
            ? t('patternEditor.preview.empty')
            : t('patternEditor.preview.summary', { total: totalDuration })}
        </p>

        <div className="flex min-h-[56px] items-stretch overflow-hidden rounded-xl border theme-border bg-[rgb(var(--surface-raised)/0.5)]">
          {steps.length === 0 ? (
            <div className="flex flex-1 items-center justify-center text-sm">
              <span className="theme-muted">{t('patternEditor.preview.empty')}</span>
            </div>
          ) : (
            patternSegments
          )}
          <button
            aria-label={t('patternEditor.form.addButton')}
            className="flex min-w-[56px] items-center justify-center border-l theme-border bg-[rgb(var(--surface-raised)/0.4)] text-xl font-semibold text-[rgb(var(--accent)/1)] transition hover:bg-[rgb(var(--surface-raised)/0.6)]"
            onClick={openAddModal}
            title={t('patternEditor.form.addButton')}
            type="button"
          >
            <span aria-hidden="true">＋</span>
          </button>
        </div>
      </Section>

      {selectedStepIndex !== null && steps[selectedStepIndex] && (
        <Section
          title={`${t('patternEditor.steps.title')} #${String(selectedStepIndex + 1)}`}
          actions={
            <button
              aria-label={t('patternEditor.steps.closeEditor')}
              className="rounded-full p-1 theme-muted hover:text-[rgb(var(--surface-contrast)/1)] hover:bg-[rgb(var(--surface-raised)/1)] transition-colors"
              onClick={() => {
                setSelectedStepIndex(null);
              }}
              type="button"
            >
              ✕
            </button>
          }
        >
          <div className="flex flex-wrap items-center gap-6 mb-4">
            <label className="flex items-center gap-3 text-sm font-medium text-[rgb(var(--surface-contrast)/0.8)]">
              <span>{labels.valueLabel}</span>
              {renderInput({
                value: steps[selectedStepIndex].value,
                onChange: newValue => {
                  handleStepValueChange(steps[selectedStepIndex].id, newValue);
                },
              })}
            </label>

            <label className="flex items-center gap-3 text-sm font-medium text-[rgb(var(--surface-contrast)/0.8)]">
              <span>{t('patternEditor.form.durationLabel')}</span>
              <div className="relative">
                <input
                  className="w-24 rounded-xl bg-[rgb(var(--surface-raised)/0.5)] theme-border border px-3 py-2 text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none text-sm"
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

          <div className="flex flex-wrap gap-2">
            <StyledButton
              disabled={selectedStepIndex === 0}
              onClick={() => {
                handleMove(steps[selectedStepIndex].id, 'up');
              }}
            >
              ← {t('patternEditor.steps.moveUp')}
            </StyledButton>
            <StyledButton
              disabled={selectedStepIndex === steps.length - 1}
              onClick={() => {
                handleMove(steps[selectedStepIndex].id, 'down');
              }}
            >
              {t('patternEditor.steps.moveDown')} →
            </StyledButton>
            <div className="flex-1" />
            <StyledButton
              onClick={() => {
                handleDuplicate(steps[selectedStepIndex].id);
              }}
            >
              {t('patternEditor.steps.duplicate')}
            </StyledButton>
            <StyledButton
              variant="danger"
              onClick={() => {
                handleRemove(steps[selectedStepIndex].id);
              }}
            >
              {t('patternEditor.steps.remove')}
            </StyledButton>
          </div>
        </Section>
      )}

      {isAddModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 px-4">
          <div
            aria-modal="true"
            className="w-full max-w-sm space-y-4 rounded-2xl theme-border border bg-[rgb(var(--surface-raised)/0.95)] p-6 shadow-xl text-[rgb(var(--surface-contrast)/1)]"
            role="dialog"
          >
            <header className="space-y-1">
              <h4 className="text-lg font-semibold">{t('patternEditor.addModal.title')}</h4>
              <p className="theme-muted text-sm">
                {t('patternEditor.preview.summary', {
                  total: totalDuration + (canConfirmModal ? parsedModalDuration : 0),
                })}
              </p>
            </header>
            <div className="space-y-4">
              <label className="flex flex-col gap-2 text-sm">
                <span className="font-medium text-[rgb(var(--surface-contrast)/0.8)]">
                  {labels.valueLabel}
                </span>
                {renderInput({
                  value: modalValue,
                  onChange: handleModalValueChange,
                  id: 'simple-pattern-modal-value',
                })}
                {labels.valueHelper && (
                  <span className="theme-muted text-xs" id="simple-pattern-modal-value-helper">
                    {labels.valueHelper}
                  </span>
                )}
              </label>
              <label className="flex flex-col gap-2 text-sm">
                <span className="font-medium text-[rgb(var(--surface-contrast)/0.8)]">
                  {t('patternEditor.form.durationLabel')}
                </span>
                <input
                  aria-describedby="simple-pattern-modal-duration-helper"
                  className="w-full rounded-xl bg-[rgb(var(--surface-raised)/0.5)] theme-border border px-3 py-2 text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none"
                  inputMode="numeric"
                  min={1}
                  onChange={handleModalDurationChange}
                  step={1}
                  type="number"
                  value={modalDurationMs}
                />
                <span className="theme-muted text-xs" id="simple-pattern-modal-duration-helper">
                  {t('patternEditor.form.durationHelper')}
                </span>
              </label>
            </div>
            <footer className="flex justify-end gap-3">
              <StyledButton variant="ghost" onClick={handleModalCancel}>
                {t('patternEditor.addModal.cancel')}
              </StyledButton>
              <StyledButton
                variant="primary"
                disabled={!canConfirmModal}
                onClick={handleModalConfirm}
              >
                {t('patternEditor.addModal.confirm')}
              </StyledButton>
            </footer>
          </div>
        </div>
      )}
    </PanelContainer>
  );
};
