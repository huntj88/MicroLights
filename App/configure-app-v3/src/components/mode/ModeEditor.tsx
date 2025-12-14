import { useTranslation } from 'react-i18next';

import { AccelTriggerEditor } from './AccelTriggerEditor';
import { PatternSelector } from './PatternSelector';
import {
  isBinaryPattern,
  isColorPattern,
  type Mode,
  type ModeAccelTrigger,
  type ModePattern,
} from '../../app/models/mode';
import { NameEditor } from '../common/NameEditor';
import { PatternPanelContainer } from '../rgb-pattern/common/PatternPanelContainer';
import { PatternSection } from '../rgb-pattern/common/PatternSection';

export type ModeAction =
  | { type: 'update-name'; name: string }
  | { type: 'update-front-pattern'; pattern: ModePattern }
  | { type: 'update-case-pattern'; pattern: ModePattern }
  | { type: 'update-triggers'; triggers: ModeAccelTrigger[] };

interface Props {
  mode: Mode;
  onChange: (mode: Mode, action: ModeAction) => void;
  patterns: ModePattern[];
}

export const ModeEditor = ({ mode, onChange, patterns }: Props) => {
  const { t } = useTranslation();

  const handleNameChange = (name: string) => {
    onChange({ ...mode, name }, { type: 'update-name', name });
  };

  const handleFrontPatternChange = (name: string) => {
    const pattern = patterns.find(p => p.name === name);
    if (pattern) {
      onChange({ ...mode, front: { pattern } }, { type: 'update-front-pattern', pattern });
    }
  };

  const handleCasePatternChange = (name: string) => {
    const pattern = patterns.find(p => p.name === name);
    if (pattern) {
      onChange({ ...mode, case: { pattern } }, { type: 'update-case-pattern', pattern });
    }
  };

  const handleTriggersChange = (triggers: ModeAccelTrigger[]) => {
    onChange(
      {
        ...mode,
        accel: triggers.length > 0 ? { triggers } : undefined,
      },
      { type: 'update-triggers', triggers },
    );
  };

  const binaryPatterns = patterns.filter(isBinaryPattern);
  const colorPatterns = patterns.filter(p => isColorPattern(p) || p.type === 'equation');

  return (
    <PatternPanelContainer>
      <NameEditor
        name={mode.name}
        onChange={handleNameChange}
        label={t('modeEditor.nameLabel')}
        placeholder={t('modeEditor.namePlaceholder')}
        helperText={t('modeEditor.nameHelper') || ' '}
      />

      <PatternSection title={t('modeEditor.patternsTitle') || 'Patterns'}>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          <PatternSelector
            label={t('modeEditor.frontLabel')}
            value={mode.front.pattern.name}
            onChange={handleFrontPatternChange}
            patterns={binaryPatterns}
          />
          <PatternSelector
            label={t('modeEditor.caseLabel')}
            value={mode.case.pattern.name}
            onChange={handleCasePatternChange}
            patterns={colorPatterns}
          />
        </div>
      </PatternSection>

      <AccelTriggerEditor
        triggers={mode.accel?.triggers ?? []}
        onChange={handleTriggersChange}
        patterns={patterns}
      />
    </PatternPanelContainer>
  );
};
