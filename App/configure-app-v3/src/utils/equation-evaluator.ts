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
  sampleRateMs = 10
): number[] => {
  const points: number[] = [];
  for (const section of sections) {
    const steps = Math.floor(section.duration / sampleRateMs);
    for (let i = 0; i < steps; i++) {

      // 't' is relative to the start of the section.
      const t = i * (sampleRateMs / 1000); // t in seconds usually
      
      const val = evaluateEquation(section.equation, t, section.duration / 1000);
      points.push(val);
    }
  }
  return points;
};
