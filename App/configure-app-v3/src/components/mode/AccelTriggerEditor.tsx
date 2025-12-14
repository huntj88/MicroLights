import { useTranslation } from 'react-i18next';

import { PatternSelector } from './PatternSelector';
import {
  isBinaryPattern,
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
        threshold: 1.5,
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

  const binaryPatterns = patterns.filter(isBinaryPattern);
  const colorPatterns = patterns.filter(p => isColorPattern(p) || p.type === 'equation');

  return (
    <Section
      title={t('modeEditor.accelTitle')}
      actions={
        <button
          onClick={addTrigger}
          className="theme-button theme-button-primary px-3 py-1 text-sm"
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
            className="bg-[rgb(var(--surface-raised)/0.5)] theme-border rounded-lg border p-4 space-y-4"
          >
            <div className="flex justify-between items-start">
              <div className="w-1/3">
                <label className="text-sm font-medium">{t('modeEditor.thresholdLabel')}</label>
                <input
                  type="number"
                  step="0.1"
                  min="0"
                  className="theme-input w-full rounded-md border px-3 py-2"
                  value={trigger.threshold}
                  onChange={e => {
                    updateTrigger(index, { threshold: parseFloat(e.target.value) || 0 });
                  }}
                />
                <p className="theme-muted text-xs mt-1">{t('modeEditor.thresholdHelper')}</p>
              </div>
              <button
                onClick={() => {
                  removeTrigger(index);
                }}
                className="text-red-500 hover:text-red-700 text-sm"
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
                patterns={binaryPatterns}
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
