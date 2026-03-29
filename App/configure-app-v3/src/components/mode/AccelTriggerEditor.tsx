import { useTranslation } from 'react-i18next';

import { PatternSelector } from './PatternSelector';
import {
  isColorPattern,
  type ModeAccelTrigger,
  type ModePattern,
} from '../../app/models/mode';
import { Section } from '../common/Section';

interface Props {
  triggers: ModeAccelTrigger[];
  onChange: (triggers: ModeAccelTrigger[]) => void;
  patterns: ModePattern[];
}

export const AccelTriggerEditor = ({ triggers, onChange, patterns }: Props) => {
  const { t } = useTranslation();

  const addTrigger = () => {
    onChange([
      ...triggers,
      {
        threshold: 150,
        front: undefined,
        case: undefined,
      },
    ]);
  };

  const removeTrigger = (index: number) => {
    const next = [...triggers];
    next.splice(index, 1);
    onChange(next);
  };

  const updateTrigger = (index: number, update: Partial<ModeAccelTrigger>) => {
    const next = [...triggers];
    next[index] = { ...next[index], ...update };
    onChange(next);
  };

  const updatePattern = (index: number, component: 'front' | 'case', patternName: string) => {
    if (!patternName) {
      const next = [...triggers];
      next[index] = {
        ...next[index],
        [component]: undefined,
      };
      onChange(next);
      return;
    }

    const pattern = patterns.find(p => p.name === patternName);
    if (!pattern) return;

    const next = [...triggers];
    const currentTrigger = next[index];

    next[index] = {
      ...currentTrigger,
      [component]: { pattern },
    };
    onChange(next);
  };

  const colorPatterns = patterns.filter(p => isColorPattern(p) || p.type === 'equation');

  return (
    <Section
      title={t('modeEditor.accelTitle')}
      actions={
        <button
          onClick={addTrigger}
          className="theme-button theme-button-primary px-3 py-2 text-sm min-h-[44px]"
        >
          {t('modeEditor.addTrigger')}
        </button>
      }
    >
      {triggers.length === 0 && (
        <p className="theme-muted text-sm italic">{t('modeEditor.noTriggers')}</p>
      )}

      <div className="space-y-4">
        {triggers.map((trigger, index) => (
          <div
            key={index}
            className="bg-[rgb(var(--surface-raised)/0.5)] theme-border rounded-lg border p-3 sm:p-4 space-y-4"
          >
            <div className="flex flex-col gap-3 sm:flex-row sm:justify-between sm:items-start">
              <div className="w-full sm:w-1/3">
                <label className="text-sm font-medium">{t('modeEditor.thresholdLabel')}</label>
                <input
                  type="number"
                  step="1"
                  min="0"
                  max="255"
                  className="theme-input w-full rounded-md border px-3 py-2 min-h-[44px]"
                  value={trigger.threshold}
                  onChange={e => {
                    const val = parseInt(e.target.value, 10);
                    updateTrigger(index, { threshold: Number.isNaN(val) ? 0 : Math.min(255, Math.max(0, val)) });
                  }}
                />
                <p className="theme-muted text-xs mt-1">{t('modeEditor.thresholdHelper')}</p>
              </div>
              <button
                onClick={() => {
                  removeTrigger(index);
                }}
                className="text-red-500 hover:text-red-700 text-sm min-h-[44px] min-w-[44px] flex items-center justify-center rounded-lg hover:bg-red-500/10 transition-colors"
              >
                {t('modeEditor.deleteTrigger')}
              </button>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <PatternSelector
                label={t('modeEditor.frontOverride')}
                value={trigger.front?.pattern.name}
                onChange={name => {
                  updatePattern(index, 'front', name);
                }}
                patterns={patterns}
              />
              <PatternSelector
                label={t('modeEditor.caseOverride')}
                value={trigger.case?.pattern.name}
                onChange={name => {
                  updatePattern(index, 'case', name);
                }}
                patterns={colorPatterns}
              />
            </div>
          </div>
        ))}
      </div>
    </Section>
  );
};
