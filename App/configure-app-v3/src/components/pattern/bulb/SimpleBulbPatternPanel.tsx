import { useTranslation } from 'react-i18next';

import {
  binaryOutputSchema,
  type BinaryOutput,
  type SimplePattern,
} from '../../../app/models/mode';
import { type SimplePatternAction, SimplePatternEditor } from '../common/SimplePatternEditor';

export type SimpleBulbPatternAction = SimplePatternAction<BinaryOutput>;

export interface SimpleBulbPatternPanelProps {
  value: SimplePattern;
  onChange: (state: SimplePattern, action: SimpleBulbPatternAction) => void;
}

const DEFAULT_VALUE: BinaryOutput = 'high';
const EMPTY_VALUE: BinaryOutput = 'low';

export const SimpleBulbPatternPanel = ({ value, onChange }: SimpleBulbPatternPanelProps) => {
  const { t } = useTranslation();

  return (
    <SimplePatternEditor<BinaryOutput>
      value={value}
      onChange={onChange}
      valueSchema={binaryOutputSchema}
      defaultValue={DEFAULT_VALUE}
      emptyValue={EMPTY_VALUE}
      renderInput={({ value, onChange }) => (
        <div className="flex items-center gap-3">
          <span
            className={`text-sm font-medium transition-colors ${
              value === 'low' ? 'text-[rgb(var(--surface-contrast))]' : 'theme-muted'
            }`}
          >
            {t('bulbPattern.low')}
          </span>
          <button
            aria-checked={value === 'high'}
            className={`relative inline-flex h-6 w-11 items-center rounded-full transition-colors focus:outline-none focus:ring-2 focus:ring-[rgb(var(--accent))] focus:ring-offset-2 ${
              value === 'high' ? 'bg-[rgb(var(--accent))]' : 'bg-[rgb(var(--surface-muted))]'
            }`}
            onClick={() => {
              onChange(value === 'high' ? 'low' : 'high');
            }}
            role="switch"
            type="button"
          >
            <span className="sr-only">{t('bulbPattern.form.stateLabel')}</span>
            <span
              className={`${
                value === 'high' ? 'translate-x-6' : 'translate-x-1'
              } inline-block h-4 w-4 transform rounded-full bg-white transition-transform`}
            />
          </button>
          <span
            className={`text-sm font-medium transition-colors ${
              value === 'high' ? 'text-[rgb(var(--surface-contrast))]' : 'theme-muted'
            }`}
          >
            {t('bulbPattern.high')}
          </span>
        </div>
      )}
      renderPreview={({ value, durationMs, isSelected, onClick, totalDuration }) => (
        <button
          aria-label={t('patternEditor.preview.segmentLabel', {
            value,
            duration: durationMs,
          })}
          aria-pressed={isSelected}
          className={`flex flex-1 items-center justify-center text-xs font-medium transition-all hover:opacity-90 focus:outline-none focus:ring-2 focus:ring-[rgb(var(--accent)/1)] focus:ring-inset ${
            isSelected
              ? 'z-10 scale-[1.02] shadow-lg ring-2 ring-[rgb(var(--accent)/1)] ring-inset'
              : ''
          } ${
            value === 'high'
              ? 'bg-[color-mix(in_srgb,rgb(var(--accent)),white_60%)] text-black'
              : 'bg-black text-white'
          }`}
          onClick={onClick}
          style={{
            flexGrow: totalDuration > 0 ? durationMs : 1,
          }}
          title={t('patternEditor.preview.segmentLabel', {
            value,
            duration: durationMs,
          })}
          type="button"
        >
          <span className="px-2 py-1">{durationMs}ms</span>
        </button>
      )}
      renderSwatch={({ value }) => (
        <div
          className={`w-full h-full ${value === 'high' ? 'bg-[rgb(var(--accent))]' : 'bg-black'}`}
          data-testid="current-state-swatch"
        />
      )}
      labels={{
        valueLabel: t('bulbPattern.form.stateLabel'),
        valueHelper: t('bulbPattern.form.stateHelper'),
      }}
    />
  );
};
