import { useMemo } from 'react';

import { type SimplePattern } from '../../../app/models/mode';

interface Props {
  pattern: SimplePattern;
  className?: string;
}

export const SimplePatternPreview = ({ pattern, className = '' }: Props) => {
  const segments = useMemo(() => {
    const sortedChanges = [...pattern.changeAt].sort((a, b) => a.ms - b.ms);
    const segs = [];

    for (let i = 0; i < sortedChanges.length; i++) {
      const current = sortedChanges[i];
      const nextMs = i === sortedChanges.length - 1 ? pattern.duration : sortedChanges[i + 1].ms;
      const duration = nextMs - current.ms;
      const widthPercent = (duration / pattern.duration) * 100;

      let backgroundColor = 'transparent';
      if (current.output === 'high') {
        backgroundColor = 'rgb(var(--accent))'; // Use theme accent for high
      } else if (current.output === 'low') {
        backgroundColor = '#000000';
      } else {
        backgroundColor = current.output;
      }

      segs.push({
        width: `${String(widthPercent)}%`,
        backgroundColor,
        key: i,
      });
    }
    return segs;
  }, [pattern]);

  return (
    <div className={`h-4 w-full flex rounded overflow-hidden border theme-border ${className}`}>
      {segments.map(seg => (
        <div
          key={seg.key}
          data-testid="pattern-segment"
          style={{ width: seg.width, backgroundColor: seg.backgroundColor }}
          className="h-full"
        />
      ))}
    </div>
  );
};
