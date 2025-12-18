import { act, renderHook } from '@testing-library/react';
import { beforeEach, describe, expect, it } from 'vitest';

import { useModeStore } from './mode-store';
import { hexColorSchema, type Mode } from '../models/mode';

const createMockMode = (name: string): Mode => ({
  name,
  front: {
    pattern: {
      type: 'simple',
      name: 'front-pattern',
      duration: 1000,
      changeAt: [{ ms: 0, output: 'high' }],
    },
  },
  case: {
    pattern: {
      type: 'simple',
      name: 'case-pattern',
      duration: 1000,
      changeAt: [{ ms: 0, output: hexColorSchema.parse('#ff0000') }],
    },
  },
});

describe('useModeStore', () => {
  beforeEach(() => {
    localStorage.clear();
    useModeStore.setState({ modes: [] });
  });

  it('starts with empty modes', () => {
    const { result } = renderHook(() => useModeStore());
    expect(result.current.modes).toEqual([]);
  });

  it('saves a new mode', () => {
    const { result } = renderHook(() => useModeStore());
    const mode = createMockMode('Test Mode');

    act(() => {
      result.current.saveMode(mode);
    });

    expect(result.current.modes).toHaveLength(1);
    expect(result.current.modes[0]).toEqual(mode);
  });

  it('updates an existing mode', () => {
    const { result } = renderHook(() => useModeStore());
    const mode = createMockMode('Test Mode');

    act(() => {
      result.current.saveMode(mode);
    });

    if (!mode.front) throw new Error('front is undefined');

    const updatedMode = {
      ...mode,
      front: {
        ...mode.front,
        pattern: { ...mode.front.pattern, duration: 2000 },
      },
    };

    act(() => {
      result.current.saveMode(updatedMode);
    });

    expect(result.current.modes).toHaveLength(1);
    expect(result.current.modes[0].front?.pattern.duration).toBe(2000);
  });

  it('deletes a mode', () => {
    const { result } = renderHook(() => useModeStore());
    const mode = createMockMode('Test Mode');

    act(() => {
      result.current.saveMode(mode);
    });

    act(() => {
      result.current.deleteMode('Test Mode');
    });

    expect(result.current.modes).toHaveLength(0);
  });

  it('gets a mode by name', () => {
    const { result } = renderHook(() => useModeStore());
    const mode = createMockMode('Test Mode');

    act(() => {
      result.current.saveMode(mode);
    });

    const retrieved = result.current.getMode('Test Mode');
    expect(retrieved).toEqual(mode);
  });

  it('returns undefined for non-existent mode', () => {
    const { result } = renderHook(() => useModeStore());
    const retrieved = result.current.getMode('Non Existent');
    expect(retrieved).toBeUndefined();
  });

  it('updates modes when a pattern is updated', () => {
    const { result } = renderHook(() => useModeStore());
    const mode = createMockMode('Test Mode');

    act(() => {
      result.current.saveMode(mode);
    });

    const originalPattern =
      mode.front?.pattern ??
      (() => {
        throw new Error('front pattern is undefined');
      })();

    // Update Duration
    const updatedPattern = {
      ...originalPattern,
      duration: 9999,
    };

    act(() => {
      result.current.updatePatternInModes(updatedPattern);
    });

    expect(result.current.modes[0].front?.pattern.duration).toBe(9999);
    // Case pattern should remain unchanged as it has a different name
    expect(result.current.modes[0].case?.pattern.duration).toBe(1000);
  });
});
