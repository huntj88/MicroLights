import { beforeEach, describe, expect, it } from 'vitest';

import { usePatternStore } from './pattern-store';
import type { SimplePattern } from '../models/mode';

const createSimplePattern = (overrides?: Partial<SimplePattern>): SimplePattern => ({
  name: 'Test Pattern',
  type: 'simple',
  duration: 100,
  changeAt: [
    { ms: 0, output: '#112233' as SimplePattern['changeAt'][number]['output'] },
  ],
  ...overrides,
});

describe('pattern-store', () => {
  beforeEach(() => {
    const { patterns, savePattern, deletePattern, getPattern } = usePatternStore.getState();
    void patterns;
    void savePattern;
    void deletePattern;
    void getPattern;
    usePatternStore.setState({ patterns: [] });
    localStorage.clear();
  });

  it('saves new patterns and retrieves them by name', () => {
    const pattern = createSimplePattern();
    usePatternStore.getState().savePattern(pattern);

    const stored = usePatternStore.getState().getPattern(pattern.name);
    expect(stored).toEqual(pattern);
    expect(stored).not.toBe(pattern);
  });

  it('overwrites patterns with the same name', () => {
    const patternA = createSimplePattern();
    const patternB = createSimplePattern({
      duration: 250,
      changeAt: [
        { ms: 0, output: '#abcdef' as SimplePattern['changeAt'][number]['output'] },
      ],
    });

    usePatternStore.getState().savePattern(patternA);
    usePatternStore.getState().savePattern(patternB);

    const stored = usePatternStore.getState().getPattern(patternA.name);
    expect(stored).toEqual(patternB);
  });

  it('deletes patterns by name', () => {
    const pattern = createSimplePattern();
    usePatternStore.getState().savePattern(pattern);

    usePatternStore.getState().deletePattern(pattern.name);

    expect(usePatternStore.getState().patterns).toHaveLength(0);
  });
});
