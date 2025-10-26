import { useEffect, useRef, useState } from 'react';

import {
  bulbModeToSegments,
  type BulbModeWaveform,
  type BulbModeWaveOutput,
} from '@/lib/bulbModeWaveform';

// Read-only renderer that matches the BulbModeWaveformEditor look (grid + 2px green polyline)
export function BulbModeWaveformMini({
  wf,
  height = 64,
}: {
  wf: BulbModeWaveform;
  height?: number;
}) {
  const svgRef = useRef<SVGSVGElement>(null);
  const [width, setWidth] = useState<number>(600);

  useEffect(() => {
    const el = svgRef.current;
    if (!el) return;
    const measure = () => setWidth(el.clientWidth || 600);
    measure();
    const ro = new ResizeObserver(measure);
    ro.observe(el);
    window.addEventListener('resize', measure);
    return () => {
      ro.disconnect();
      window.removeEventListener('resize', measure);
    };
  }, []);

  const segs = bulbModeToSegments(wf);
  const leftPad = 12;
  const rightPad = 12;
  const per = Math.max(1, (width - leftPad - rightPad) / wf.totalTicks);
  const yFor = (out: BulbModeWaveOutput) => (out === 'high' ? 24 : height - 24);

  const points = segs
    .flatMap(s => [
      [leftPad + s.from * per, yFor(s.output)],
      [leftPad + s.to * per, yFor(s.output)],
    ])
    .map(p => p.join(','))
    .join(' ');

  return (
    <svg
      ref={svgRef}
      width="100%"
      height={height}
      role="img"
      aria-label="Bulb mode waveform preview"
      className="rounded"
    >
      {/* center dashed line */}
      <line
        x1={leftPad}
        y1={height / 2}
        x2={width - rightPad}
        y2={height / 2}
        stroke="#334155"
        strokeDasharray="4 4"
      />

      {/* waveform polyline (uniform stroke) */}
      <polyline
        fill="none"
        stroke="#22c55e"
        strokeWidth={2}
        strokeLinecap="butt"
        strokeLinejoin="miter"
        points={points}
      />
    </svg>
  );
}
