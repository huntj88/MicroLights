export const evaluateEquation = (equation: string, t: number, duration: number): number => {
  try {
    // Create a function that takes 't' and 'Duration' and exposes Math functions
    // We replace 't' with the value and 'Duration' with the duration value in the scope
    // But a cleaner way is to use new Function with arguments.
    
    // We want to support "sin(t)", "exp(t)", etc. directly.
    // So we can use a `with` statement or destructure Math.
    // `with` is deprecated/strict mode forbidden.
    
    // Let's construct a function body that destructures Math.
    const mathKeys = Object.getOwnPropertyNames(Math);
    const mathArgs = mathKeys.map(key => `const ${key} = Math.${key};`).join('\n');
    
    const body = `
      ${mathArgs}
      return ${equation};
    `;
    
    // eslint-disable-next-line @typescript-eslint/no-implied-eval
    const func = new Function('t', 'Duration', 'Math', body);
    // eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call
    const result: number = func(t, duration, Math);
    
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
  sampleRateMs = 10
): number[] => {
  const points: number[] = [];
  const channelDurationMs = sections.reduce((acc, s) => acc + s.duration, 0);
  const steps = Math.floor(totalDurationMs / sampleRateMs);

  if (channelDurationMs === 0) {
    return new Array(steps).fill(0);
  }

  for (let i = 0; i < steps; i++) {
    const globalTMs = i * sampleRateMs;
    let tMs = globalTMs;

    if (loop) {
      tMs = tMs % channelDurationMs;
    }

    // Find active section
    let activeSection = sections[sections.length - 1];
    let sectionStartTime = channelDurationMs - activeSection.duration;
    let found = false;

    let accumulated = 0;
    for (const section of sections) {
      if (tMs < accumulated + section.duration) {
        activeSection = section;
        sectionStartTime = accumulated;
        found = true;
        break;
      }
      accumulated += section.duration;
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
    const durationSec = activeSection.duration / 1000;

    const val = evaluateEquation(activeSection.equation, tSec, durationSec);
    points.push(val);
  }
  return points;
};
