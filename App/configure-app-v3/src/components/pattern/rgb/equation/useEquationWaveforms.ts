import { useMemo } from 'react';

import { type EquationPattern } from '../../../../app/models/mode';
import { generateWaveformPoints } from '../../../../utils/equation-evaluator';

export const useEquationWaveforms = (pattern: EquationPattern, sampleRate = 10) => {
  const calculateChannelDuration = (sections: { duration: number }[]) =>
    sections.reduce(
      (acc, section) => acc + (Number.isNaN(section.duration) ? 0 : section.duration),
      0,
    );

  const redDuration = useMemo(
    () => calculateChannelDuration(pattern.red.sections),
    [pattern.red.sections],
  );
  const greenDuration = useMemo(
    () => calculateChannelDuration(pattern.green.sections),
    [pattern.green.sections],
  );
  const blueDuration = useMemo(
    () => calculateChannelDuration(pattern.blue.sections),
    [pattern.blue.sections],
  );

  const totalDuration = Math.max(pattern.duration, redDuration, greenDuration, blueDuration, 1000);

  const redPoints = useMemo(
    () =>
      generateWaveformPoints(
        pattern.red.sections,
        totalDuration,
        pattern.red.loopAfterDuration,
        sampleRate,
      ),
    [pattern.red.sections, totalDuration, pattern.red.loopAfterDuration, sampleRate],
  );
  const greenPoints = useMemo(
    () =>
      generateWaveformPoints(
        pattern.green.sections,
        totalDuration,
        pattern.green.loopAfterDuration,
        sampleRate,
      ),
    [pattern.green.sections, totalDuration, pattern.green.loopAfterDuration, sampleRate],
  );
  const bluePoints = useMemo(
    () =>
      generateWaveformPoints(
        pattern.blue.sections,
        totalDuration,
        pattern.blue.loopAfterDuration,
        sampleRate,
      ),
    [pattern.blue.sections, totalDuration, pattern.blue.loopAfterDuration, sampleRate],
  );

  return {
    redPoints,
    greenPoints,
    bluePoints,
    totalDuration,
  };
};
