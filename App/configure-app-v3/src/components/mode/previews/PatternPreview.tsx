import { EquationPatternPreview } from './EquationPatternPreview';
import { SimplePatternPreview } from './SimplePatternPreview';
import { type ModePattern } from '../../../app/models/mode';

interface Props {
  pattern: ModePattern;
  className?: string;
}

export const PatternPreview = ({ pattern, className = '' }: Props) => {
  return (
    <div className={className}>
      {pattern.type === 'simple' ? (
        <SimplePatternPreview pattern={pattern} />
      ) : (
        <EquationPatternPreview pattern={pattern} />
      )}
    </div>
  );
};
