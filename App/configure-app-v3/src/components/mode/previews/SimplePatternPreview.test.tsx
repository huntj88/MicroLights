import { render, screen } from '@testing-library/react';
import { describe, expect, it } from 'vitest';

import { SimplePatternPreview } from './SimplePatternPreview';
import { hexColorSchema, type SimplePattern } from '../../../app/models/mode';

describe('SimplePatternPreview', () => {
  it('renders segments with correct widths and colors', () => {
    const pattern: SimplePattern = {
      type: 'simple',
      name: 'Test Pattern',
      duration: 1000,
      changeAt: [
        { ms: 0, output: hexColorSchema.parse('#ff0000') },
        { ms: 500, output: hexColorSchema.parse('#00ff00') },
      ],
    };

    render(<SimplePatternPreview pattern={pattern} />);
    const segments = screen.getAllByTestId('pattern-segment');

    expect(segments).toHaveLength(2);

    // First segment: 0-500ms (50%)
    expect(segments[0]).toHaveStyle({ width: '50%', backgroundColor: '#ff0000' });

    // Second segment: 500-1000ms (50%)
    expect(segments[1]).toHaveStyle({ width: '50%', backgroundColor: '#00ff00' });
  });

  it('handles bulb pattern values (high/low)', () => {
    const pattern: SimplePattern = {
      type: 'simple',
      name: 'Bulb Pattern',
      duration: 200,
      changeAt: [
        { ms: 0, output: 'high' },
        { ms: 100, output: 'low' },
      ],
    };

    render(<SimplePatternPreview pattern={pattern} />);
    const segments = screen.getAllByTestId('pattern-segment');

    expect(segments).toHaveLength(2);

    // High -> Accent color (checking for style presence, exact color depends on CSS var)
    expect(segments[0]).toHaveStyle({ width: '50%', backgroundColor: 'rgb(var(--accent))' });

    // Low -> Black
    expect(segments[1]).toHaveStyle({ width: '50%', backgroundColor: '#000000' });
  });
});
