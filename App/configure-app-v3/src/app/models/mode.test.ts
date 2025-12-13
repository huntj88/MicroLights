import { describe, expect, it } from 'vitest';

import {
  hexColorSchema,
  isBinaryPattern,
  isColorPattern,
  modeDocumentSchema,
  parseModeDocument,
  simplePatternSchema,
} from './mode';

describe('simplePatternSchema', () => {
  it('validates a correct pattern', () => {
    const pattern = {
      type: 'simple',
      name: 'Valid Pattern',
      duration: 1000,
      changeAt: [
        { ms: 0, output: hexColorSchema.parse('#ff0000') },
        { ms: 500, output: hexColorSchema.parse('#00ff00') },
      ],
    };
    const result = simplePatternSchema.safeParse(pattern);
    expect(result.success).toBe(true);
  });

  it('fails when the last step has zero duration', () => {
    const pattern = {
      type: 'simple',
      name: 'Last Step Zero Duration',
      duration: 1000,
      changeAt: [
        { ms: 0, output: hexColorSchema.parse('#ff0000') },
        { ms: 1000, output: hexColorSchema.parse('#00ff00') }, // Last step starts at end duration
      ],
    };
    const result = simplePatternSchema.safeParse(pattern);
    expect(result.success).toBe(false);
    if (!result.success) {
      expect(result.error.issues[0].message).toContain(
        'validation.pattern.simple.stepDurationZero',
      );
    }
  });
});

describe('modeDocumentSchema', () => {
  it('parses a mode with accelerometer triggers and binary outputs', () => {
    const result = parseModeDocument({
      mode: {
        name: 'accel Pattern',
        front: {
          pattern: {
            type: 'simple',
            name: 'front binary pattern',
            duration: 3,
            changeAt: [
              { ms: 0, output: 'high' },
              { ms: 2, output: 'low' },
            ],
          },
        },
        case: {
          pattern: {
            type: 'simple',
            name: 'case color pattern',
            duration: 3,
            changeAt: [
              { ms: 0, output: '#ffaa00' },
              { ms: 2, output: '#005110' },
            ],
          },
        },
        accel: {
          triggers: [
            {
              threshold: 2,
              front: {
                pattern: {
                  type: 'simple',
                  name: 'fullOn',
                  duration: 1,
                  changeAt: [{ ms: 0, output: 'high' }],
                },
              },
              case: {
                pattern: {
                  type: 'simple',
                  name: 'flash white',
                  duration: 3,
                  changeAt: [
                    { ms: 0, output: '#ffffff' },
                    { ms: 2, output: '#000000' },
                  ],
                },
              },
            },
          ],
        },
      },
    });

    expect(result.mode.name).toBe('accel Pattern');
    expect(isBinaryPattern(result.mode.front.pattern)).toBe(true);
    expect(isColorPattern(result.mode.case.pattern)).toBe(true);
    expect(result.mode.accel?.triggers).toHaveLength(1);
  });

  it('parses modes that use RGB outputs for both LEDs', () => {
    const { mode } = parseModeDocument({
      mode: {
        name: 'has rgb patterns for both',
        front: {
          pattern: {
            type: 'simple',
            name: 'front color pattern',
            duration: 3,
            changeAt: [
              { ms: 0, output: '#222222' },
              { ms: 2, output: '#773377' },
            ],
          },
        },
        case: {
          pattern: {
            type: 'simple',
            name: 'case color pattern',
            duration: 3,
            changeAt: [
              { ms: 0, output: '#ffaa00' },
              { ms: 2, output: '#005110' },
            ],
          },
        },
      },
    });

    expect(isColorPattern(mode.front.pattern)).toBe(true);
    expect(isColorPattern(mode.case.pattern)).toBe(true);
  });

  it('rejects invalid hex colors', () => {
    const result = modeDocumentSchema.safeParse({
      mode: {
        name: 'bad color',
        front: {
          pattern: {
            type: 'simple',
            name: 'bad front',
            duration: 1,
            changeAt: [{ ms: 0, output: '#fff' }],
          },
        },
        case: {
          pattern: {
            type: 'simple',
            name: 'case',
            duration: 1,
            changeAt: [{ ms: 0, output: '#ffaa00' }],
          },
        },
      },
    });

    expect(result.success).toBe(false);
    if (!result.success) {
      expect(result.error.issues[0]?.message).toBe('validation.pattern.simple.hexColor');
    }
  });

  it('rejects duplicate timestamps within a pattern', () => {
    const result = modeDocumentSchema.safeParse({
      mode: {
        name: 'duplicate timestamps',
        front: {
          pattern: {
            type: 'simple',
            name: 'front pattern',
            duration: 2,
            changeAt: [
              { ms: 0, output: 'high' },
              { ms: 0, output: 'low' },
            ],
          },
        },
        case: {
          pattern: {
            type: 'simple',
            name: 'case pattern',
            duration: 2,
            changeAt: [
              { ms: 0, output: '#ffaa00' },
              { ms: 1, output: '#005110' },
            ],
          },
        },
      },
    });

    expect(result.success).toBe(false);
    if (!result.success) {
      expect(
        result.error.issues.some(
          issue => issue.message === 'validation.pattern.simple.timestamp.unique',
        ),
      ).toBe(true);
    }
  });

  it('requires accelerometer triggers to target at least one LED', () => {
    const result = modeDocumentSchema.safeParse({
      mode: {
        name: 'invalid accel trigger',
        front: {
          pattern: {
            type: 'simple',
            name: 'front pattern',
            duration: 1,
            changeAt: [{ ms: 0, output: 'high' }],
          },
        },
        case: {
          pattern: {
            type: 'simple',
            name: 'case pattern',
            duration: 1,
            changeAt: [{ ms: 0, output: '#ffaa00' }],
          },
        },
        accel: {
          triggers: [
            {
              threshold: 1,
            },
          ],
        },
      },
    });

    expect(result.success).toBe(false);
    if (!result.success) {
      expect(
        result.error.issues.some(issue => issue.message === 'validation.accel.componentRequired'),
      ).toBe(true);
    }
  });

  it('parses a mode with equation patterns', () => {
    const result = parseModeDocument({
      mode: {
        name: 'equation mode',
        front: {
          pattern: {
            type: 'equation',
            name: 'sine wave',
            duration: 1000,
            red: {
              sections: [
                {
                  equation: '127 + 127 * sin(2 * PI * t / 1000)',
                  duration: 1000,
                },
              ],
              loopAfterDuration: true,
            },
            green: {
              sections: [],
              loopAfterDuration: true,
            },
            blue: {
              sections: [],
              loopAfterDuration: true,
            },
          },
        },
        case: {
          pattern: {
            type: 'simple',
            name: 'simple case',
            duration: 100,
            changeAt: [{ ms: 0, output: '#000000' }],
          },
        },
      },
    });

    expect(result.mode.front.pattern.type).toBe('equation');
    if (result.mode.front.pattern.type === 'equation') {
      expect(result.mode.front.pattern.red.sections).toHaveLength(1);
      expect(result.mode.front.pattern.red.sections[0].equation).toContain('sin');
    }
  });
});
