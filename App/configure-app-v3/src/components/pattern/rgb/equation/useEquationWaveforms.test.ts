import { renderHook } from '@testing-library/react';
import { describe, expect, it } from 'vitest';

import { useEquationWaveforms } from './useEquationWaveforms';
import { createDefaultEquationPattern } from '../../../../app/models/mode';

describe('useEquationWaveforms', () => {
  it('calculates totalDuration based on the maximum of pattern duration and channel durations where channels are longer', () => {
    const pattern = createDefaultEquationPattern();
    pattern.duration = 500; // Short pattern duration

    // Add a long section to red channel
    pattern.red.sections = [{ equation: '1', duration: 2000 }];
    pattern.green.sections = [{ equation: '1', duration: 100 }];
    pattern.blue.sections = [];

    const { result } = renderHook(() => useEquationWaveforms(pattern));

    // Should be max(500, 2000, 100, 0, 1000) = 2000
    expect(result.current.totalDuration).toBe(2000);
  });

  it('defaults to 1000ms if everything is shorter', () => {
    const pattern = createDefaultEquationPattern();
    pattern.duration = 500;
    pattern.red.sections = [{ equation: '1', duration: 100 }];

    const { result } = renderHook(() => useEquationWaveforms(pattern));

    // Should be max(500, 100, 0, 0, 1000) = 1000
    expect(result.current.totalDuration).toBe(1000);
  });

  it('respects pattern duration if it is the longest', () => {
    const pattern = createDefaultEquationPattern();
    pattern.duration = 5000;
    pattern.red.sections = [{ equation: '1', duration: 2000 }];

    const { result } = renderHook(() => useEquationWaveforms(pattern));

    expect(result.current.totalDuration).toBe(5000);
  });
});
