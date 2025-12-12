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
  totalDuration: number,
  sampleRateMs = 10
): number[] => {
  const points: number[] = [];
  let currentT = 0;
  
  // We need to map global time 't' to the section's local time if needed, 
  // but usually 't' in these equations refers to time within the section or global time?
  // The prompt says: "Equation 255 * exp(-0.1*t) (starts at full red, decays)."
  // This implies 't' is relative to the start of the section.
  
  for (const section of sections) {
    const steps = Math.floor(section.duration / sampleRateMs);
    for (let i = 0; i < steps; i++) {
      const t = i * (sampleRateMs / 1000); // t in seconds usually? Or ms?
      // Let's assume t is in seconds for equations like exp(-0.1*t) to make sense comfortably,
      // or the user specifies. Let's assume seconds for the equation variable 't'.
      
      const val = evaluateEquation(section.equation, t, section.duration / 1000);
      points.push(val);
    }
    currentT += section.duration;
  }
  
  // Fill remaining if any (though we iterate by sections)
  return points;
};
