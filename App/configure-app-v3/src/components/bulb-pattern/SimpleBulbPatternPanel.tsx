import { useTranslation } from 'react-i18next';

import { binaryOutputSchema, type BinaryOutput, type SimplePattern } from '../../app/models/mode';
import { type SimplePatternAction, SimplePatternEditor } from '../simple-pattern/SimplePatternEditor';

export type SimpleBulbPatternAction = SimplePatternAction<BinaryOutput>;

export interface SimpleBulbPatternPanelProps {
  value: SimplePattern;
  onChange: (state: SimplePattern, action: SimpleBulbPatternAction) => void;
}

const DEFAULT_VALUE: BinaryOutput = 'high';

export const SimpleBulbPatternPanel = ({ value, onChange }: SimpleBulbPatternPanelProps) => {
  const { t } = useTranslation();

  return (
    <SimplePatternEditor<BinaryOutput>
      value={value}
      onChange={onChange}
      valueSchema={binaryOutputSchema}
      defaultValue={DEFAULT_VALUE}
      renderInput={({ value, onChange }) => (
        <div className="flex items-center gap-2">
          <button
            className={`rounded-md border px-3 py-1 transition-colors ${
              value === 'high'
                ? 'border-white bg-white text-black'
                : 'border-gray-600 bg-transparent text-white hover:bg-white/10'
            }`}
            onClick={() => {
              onChange('high');
            }}
            type="button"
          >
            {t('bulbPattern.high')}
          </button>
          <button
            className={`rounded-md border px-3 py-1 transition-colors ${
              value === 'low'
                ? 'border-white bg-black text-white'
                : 'border-gray-600 bg-transparent text-white hover:bg-white/10'
            }`}
            onClick={() => {
              onChange('low');
            }}
            type="button"
          >
            {t('bulbPattern.low')}
          </button>
        </div>
      )}
      renderPreview={({ value, durationMs, isSelected, onClick, totalDuration }) => (
        <button
          aria-label={t('bulbPattern.preview.segmentLabel', {
            state: value,
            duration: durationMs,
          })}
          aria-pressed={isSelected}
          className={`flex flex-1 items-center justify-center text-xs font-medium transition-all hover:opacity-90 focus:outline-none focus:ring-2 focus:ring-[rgb(var(--accent)/1)] focus:ring-inset ${
            isSelected
              ? 'z-10 scale-[1.02] shadow-lg ring-2 ring-[rgb(var(--accent)/1)] ring-inset'
              : ''
          } ${value === 'high' ? 'bg-white text-black' : 'bg-black text-white'}`}
          onClick={onClick}
          style={{
            flexGrow: totalDuration > 0 ? durationMs : 1,
          }}
          title={t('bulbPattern.preview.segmentLabel', {
            state: value,
            duration: durationMs,
          })}
          type="button"
        >
          <span className="px-2 py-1 mix-blend-difference">{durationMs}ms</span>
        </button>
      )}
      labels={{
        valueLabel: t('bulbPattern.form.stateLabel'),
        valueHelper: t('bulbPattern.form.stateHelper'),
      }}
    />
  );
};
