import { beforeEach, describe, expect, it } from 'vitest';

import { usePatternStore } from './pattern-store';
import { createDefaultEquationPattern, type EquationPattern, type SimplePattern } from '../models/mode';

const createSimplePattern = (overrides?: Partial<SimplePattern>): SimplePattern => ({
  name: 'Test Pattern',
  type: 'simple',
  duration: 100,
  changeAt: [
    { ms: 0, output: '#112233' as SimplePattern['changeAt'][number]['output'] },
  ],
  ...overrides,
});

const createEquationPattern = (overrides?: Partial<EquationPattern>): EquationPattern => ({
  ...createDefaultEquationPattern(),
  name: 'Test Equation Pattern',
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

  describe('simple patterns', () => {
    it('saves new simple patterns and retrieves them by name', () => {
      const pattern = createSimplePattern();
      usePatternStore.getState().savePattern(pattern);

      const stored = usePatternStore.getState().getPattern(pattern.name);
      expect(stored).toEqual(pattern);
      expect(stored).not.toBe(pattern);
    });

    it('overwrites simple patterns with the same name', () => {
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

    it('deletes simple patterns by name', () => {
      const pattern = createSimplePattern();
      usePatternStore.getState().savePattern(pattern);

      usePatternStore.getState().deletePattern(pattern.name);

      expect(usePatternStore.getState().patterns).toHaveLength(0);
    });
  });

  describe('equation patterns', () => {
    it('saves new equation patterns and retrieves them by name', () => {
      const pattern = createEquationPattern();
      // Add some sections to verify deep cloning
      pattern.red.sections.push({ equation: 'sin(t)', duration: 1000 });
      
      usePatternStore.getState().savePattern(pattern);

      const stored = usePatternStore.getState().getPattern(pattern.name);
      expect(stored).toEqual(pattern);
      expect(stored).not.toBe(pattern);
      
      if (stored?.type === 'equation') {
        expect(stored.red.sections).not.toBe(pattern.red.sections);
        expect(stored.red.sections[0]).not.toBe(pattern.red.sections[0]);
      }
    });

    it('overwrites equation patterns with the same name', () => {
      const patternA = createEquationPattern();
      const patternB = createEquationPattern({
        duration: 2000,
        blue: {
            sections: [{ equation: 'cos(t)', duration: 2000 }],
            loopAfterDuration: false
        }
      });

      usePatternStore.getState().savePattern(patternA);
      usePatternStore.getState().savePattern(patternB);

      const stored = usePatternStore.getState().getPattern(patternA.name);
      expect(stored).toEqual(patternB);
    });

    it('deletes equation patterns by name', () => {
      const pattern = createEquationPattern();
      usePatternStore.getState().savePattern(pattern);

      usePatternStore.getState().deletePattern(pattern.name);

      expect(usePatternStore.getState().patterns).toHaveLength(0);
    });
  });
});
