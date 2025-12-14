import { useTranslation } from 'react-i18next';
import { z } from 'zod';

import { hexColorSchema, type HexColor, type SimplePattern } from '../../../app/models/mode';
import { type SimplePatternAction, SimplePatternEditor } from '../common/SimplePatternEditor';

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

const DEFAULT_COLOR: HexColor = hexColorSchema.parse('#ff7b00');

export const SimpleRgbPatternPanel = ({ value, onChange }: SimpleRgbPatternPanelProps) => {
  const { t } = useTranslation();

  const handleEditorChange = (state: SimplePattern, action: SimplePatternAction<HexColor>) => {
    let mappedAction: SimpleRgbPatternAction;

    if (action.type === 'add-step' || action.type === 'update-step') {
      mappedAction = {
        ...action,
        step: {
          id: action.step.id,
          color: action.step.value,
          durationMs: action.step.durationMs,
        },
      } as SimpleRgbPatternAction;
    } else if (action.type === 'duplicate-step') {
      mappedAction = {
        ...action,
        newStep: {
          id: action.newStep.id,
          color: action.newStep.value,
          durationMs: action.newStep.durationMs,
        },
      } as SimpleRgbPatternAction;
    } else {
      mappedAction = action as SimpleRgbPatternAction;
    }

    onChange(state, mappedAction);
  };

  return (
    <SimplePatternEditor<HexColor>
      value={value}
      onChange={handleEditorChange}
      // Double cast required: ZodBranded types don't strictly satisfy the generic ZodType constraint in TS
      valueSchema={hexColorSchema as unknown as z.ZodType<HexColor>}
      defaultValue={DEFAULT_COLOR}
      idPrefix="rgb-step-"
      renderInput={({ value, onChange }) => (
        <>
          <input
            className="h-10 w-10 rounded-full border theme-border cursor-pointer"
            onChange={event => {
              onChange(event.target.value as HexColor);
            }}
            type="color"
            value={value}
          />
          <span className="font-mono uppercase theme-muted">{value}</span>
        </>
      )}
      renderPreview={({ value, durationMs, isSelected, onClick, totalDuration }) => (
        <button
          aria-label={t('patternEditor.preview.segmentLabel', {
            value,
            duration: durationMs,
          })}
          aria-pressed={isSelected}
          className={`flex flex-1 items-center justify-center text-xs font-medium text-white transition-all hover:opacity-90 focus:outline-none focus:ring-2 focus:ring-[rgb(var(--accent)/1)] focus:ring-inset ${
            isSelected
              ? 'z-10 scale-[1.02] shadow-lg ring-2 ring-[rgb(var(--accent)/1)] ring-inset'
              : ''
          }`}
          onClick={onClick}
          style={{
            backgroundColor: value,
            flexGrow: totalDuration > 0 ? durationMs : 1,
          }}
          title={t('patternEditor.preview.segmentLabel', {
            value,
            duration: durationMs,
          })}
          type="button"
        >
          <span className="px-2 py-1 mix-blend-difference">{durationMs}ms</span>
        </button>
      )}
      labels={{
        valueLabel: t('rgbPattern.simple.form.colorLabel'),
        valueHelper: t('rgbPattern.simple.form.colorHelper'),
      }}
    />
  );
};
