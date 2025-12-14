import { type EquationPattern } from '../../../app/models/mode';
import { ColorPreview } from '../../pattern/rgb/equation/ColorPreview';
import { useEquationWaveforms } from '../../pattern/rgb/equation/useEquationWaveforms';

interface Props {
  pattern: EquationPattern;
  className?: string;
}

export const EquationPatternPreview = ({ pattern, className = '' }: Props) => {
  const { redPoints, greenPoints, bluePoints, totalDuration } = useEquationWaveforms(pattern);

  return (
    <div className={`h-4 w-full rounded overflow-hidden border theme-border ${className}`}>
      <ColorPreview
        redPoints={redPoints}
        greenPoints={greenPoints}
        bluePoints={bluePoints}
        currentTime={0} // Static preview
        totalDuration={totalDuration}
        showSwatch={false}
      />
    </div>
  );
};
