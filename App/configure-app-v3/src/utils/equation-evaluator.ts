import { evaluate } from 'mathjs';

export const evaluateEquation = (equation: string, t: number, duration: number): number => {
  try {
    const scope = {
      t,
      Duration: duration,
      Math: Math,
    };

    const result = Number(evaluate(equation, scope));

    // Clamp to 0-255
    return Math.max(0, Math.min(255, result));
  } catch (e) {
    console.error('Error evaluating equation:', e);
    return 0;
  }
};

export const generateWaveformPoints = (
  sections: { equation: string; duration: number }[],
  totalDurationMs: number,
  loop: boolean,
  sampleRateMs = 10,
): number[] => {
  const points: number[] = [];
  const channelDurationMs = sections.reduce(
    (acc, s) => acc + (Number.isNaN(s.duration) ? 0 : s.duration),
    0,
  );
  const steps = Math.floor(totalDurationMs / sampleRateMs);

  if (channelDurationMs === 0) {
    return new Array<number>(steps).fill(0);
  }

  for (let i = 0; i < steps; i++) {
    const globalTMs = i * sampleRateMs;
    let tMs = globalTMs;

    if (loop) {
      tMs = tMs % channelDurationMs;
    }

    // Find active section
    let activeSection = sections[sections.length - 1];
    let sectionStartTime =
      channelDurationMs - (Number.isNaN(activeSection.duration) ? 0 : activeSection.duration);

    let accumulated = 0;
    for (const section of sections) {
      const duration = Number.isNaN(section.duration) ? 0 : section.duration;
      if (tMs < accumulated + duration) {
        activeSection = section;
        sectionStartTime = accumulated;
        break;
      }
      accumulated += duration;
    }

    // Calculate t relative to the section start (in seconds)
    // If not found (and not looping), we are extending the last section.
    // tMs is the global time (because we didn't modulo it if !loop).
    // But wait, if !loop, tMs is globalTMs.
    // If we didn't find it in the loop, it means tMs >= channelDurationMs.
    // So we use the last section.
    // The last section started at `channelDurationMs - lastSection.duration`.
    // So local time is `tMs - sectionStartTime`.

    // If found, tMs is within the section (or modulo'd time).
    // local time is `tMs - sectionStartTime`.

    const tSec = (tMs - sectionStartTime) / 1000;
    const durationSec = (Number.isNaN(activeSection.duration) ? 0 : activeSection.duration) / 1000;

    const val = evaluateEquation(activeSection.equation, tSec, durationSec);
    points.push(val);
  }
  return points;
};
