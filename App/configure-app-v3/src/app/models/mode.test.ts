import { describe, expect, it } from 'vitest';

import {
  isBinaryPattern,
  isColorPattern,
  modeDocumentSchema,
  parseModeDocument,
} from './mode';

describe('modeDocumentSchema', () => {
  it('parses a mode with accelerometer triggers and binary outputs', () => {
    const result = parseModeDocument({
      mode: {
        name: 'accel Pattern',
        front: {
          pattern: {
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
                  name: 'fullOn',
                  duration: 1,
                  changeAt: [{ ms: 0, output: 'high' }],
                },
              },
              case: {
                pattern: {
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
            name: 'bad front',
            duration: 1,
            changeAt: [{ ms: 0, output: '#fff' }],
          },
        },
        case: {
          pattern: {
            name: 'case',
            duration: 1,
            changeAt: [{ ms: 0, output: '#ffaa00' }],
          },
        },
      },
    });

    expect(result.success).toBe(false);
    if (!result.success) {
      expect(result.error.issues[0]?.message).toContain('6-digit hexadecimal');
    }
  });

  it('rejects duplicate timestamps within a pattern', () => {
    const result = modeDocumentSchema.safeParse({
      mode: {
        name: 'duplicate timestamps',
        front: {
          pattern: {
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
      expect(result.error.issues.some((issue) => issue.message.includes('unique'))).toBe(true);
    }
  });

  it('requires accelerometer triggers to target at least one LED', () => {
    const result = modeDocumentSchema.safeParse({
      mode: {
        name: 'invalid accel trigger',
        front: {
          pattern: {
            name: 'front pattern',
            duration: 1,
            changeAt: [{ ms: 0, output: 'high' }],
          },
        },
        case: {
          pattern: {
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
      expect(result.error.issues.some((issue) => issue.message.includes('at least one LED'))).toBe(
        true,
      );
    }
  });
});
