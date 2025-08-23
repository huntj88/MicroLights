import { useRef, useState, useEffect } from 'react';
import { toast } from 'sonner';

import { zWaveform, type Waveform, type WavePoint, toSegments, type WaveOutput } from '@/lib/waveform';

type Props = {
  value: Waveform;
  onChange: (wf: Waveform) => void;
  height?: number;
};

// Simple square-wave timeline editor with draggable markers
export function WaveformEditor({ value, onChange, height = 160 }: Props) {
  const [dragIndex, setDragIndex] = useState<number | null>(null);
  const svgRef = useRef<SVGSVGElement>(null);

  // keep latest draft while dragging (state to trigger re-render)
  const [dragPreview, setDragPreview] = useState<WavePoint[] | null>(null);
  // suppress stray click after a drag
  const suppressNextClickRef = useRef<boolean>(false);

  // history for undo/redo
  const [past, setPast] = useState<Waveform[]>([]);
  const [future, setFuture] = useState<Waveform[]>([]);

  useEffect(() => {
    setFuture([]);
  }, [value.name, value.totalTicks, value.changeAt]);

  function pxPerTick(width: number) {
    return Math.max(1, (width - 24) / value.totalTicks);
  }

  function yFor(output: 'high' | 'low') {
    return output === 'high' ? 24 : height - 24;
  }

  function pushHistory(prev: Waveform) {
    setPast(p => [...p, prev]);
    setFuture([]);
  }

  function commit(points: WavePoint[]) {
    const next: Waveform = { ...value, changeAt: points };
    const res = zWaveform.safeParse(next);
    if (!res.success) {
      toast.error(res.error.issues[0]?.message ?? 'Invalid waveform');
      return;
    }
    pushHistory(value);
    onChange(res.data);
  }

  function onPointerDown(e: React.PointerEvent<SVGGElement>, idx: number) {
    e.stopPropagation();
    e.preventDefault();
    try {
      // Capture on the marker element so we reliably get move/up
      e.currentTarget.setPointerCapture?.(e.pointerId);
    } catch {
      // no-op
    }
    setDragIndex(idx);
    setDragPreview(value.changeAt);
    suppressNextClickRef.current = false;
  }

  function normalizeTicks(points: WavePoint[], totalTicks: number): WavePoint[] {
    if (points.length === 0) return points;
    const out = points.map(p => ({ ...p }));
    // enforce first at 0
    out[0].tick = 0;
    // ensure strictly increasing forward
    for (let i = 1; i < out.length; i++) {
      if (out[i].tick <= out[i - 1].tick) out[i].tick = out[i - 1].tick + 1;
    }
    // if last exceeds max, shift left as needed while keeping order
    const maxTick = totalTicks - 1;
    const overflow = out[out.length - 1].tick - maxTick;
    if (overflow > 0) {
      for (let i = 1; i < out.length; i++) {
        out[i].tick = Math.max(i, out[i].tick - overflow);
      }
    }
    // final pass to guarantee increasing and clamp into range
    for (let i = 1; i < out.length; i++) {
      if (out[i].tick <= out[i - 1].tick) out[i].tick = out[i - 1].tick + 1;
    }
    for (let i = 0; i < out.length; i++) {
      out[i].tick = Math.max(0, Math.min(maxTick, out[i].tick));
    }
    return out;
  }

  function onPointerMove(e: React.PointerEvent<SVGSVGElement>) {
    if (dragIndex == null || !svgRef.current) return;
    e.preventDefault();
    suppressNextClickRef.current = true; // mark that a drag occurred
    const bounds = svgRef.current.getBoundingClientRect();
    const width = bounds.width;
    const x = Math.min(Math.max(e.clientX - bounds.left, 12), width - 12);
    const per = pxPerTick(width);
    let tick = Math.round((x - 12) / per);

    // lock the first marker at tick 0 to satisfy schema
    if (dragIndex === 0) tick = 0;

    const y = e.clientY - bounds.top;
    const output: WaveOutput = y < height / 2 ? 'high' : 'low';

    let next: WavePoint[] = value.changeAt.map((p, i) => (i === dragIndex ? { tick, output } : p));
    next = normalizeTicks(next, value.totalTicks);

    // update local preview only; commit on pointer up
    setDragPreview(next);
  }

  function onPointerUp(e: React.PointerEvent<SVGSVGElement>) {
    e.stopPropagation();
    if (dragIndex == null) return;
    const points = dragPreview ?? value.changeAt;
    setDragPreview(null);
    setDragIndex(null);
    suppressNextClickRef.current = true;
    commit(points);
  }

  function onSvgClick(e: React.MouseEvent<SVGSVGElement>) {
    if (!svgRef.current) return;
    if (dragIndex != null) return; // ignore if currently dragging
    if (suppressNextClickRef.current) {
      suppressNextClickRef.current = false;
      return;
    }
    // ignore clicks originating from children (e.g., markers)
    if (e.target !== e.currentTarget) return;
    const bounds = svgRef.current.getBoundingClientRect();
    const width = bounds.width;
    const x = Math.min(Math.max(e.clientX - bounds.left, 12), width - 12);
    const y = e.clientY - bounds.top;
    const per = pxPerTick(width);
    const tick = Math.max(0, Math.min(value.totalTicks - 1, Math.round((x - 12) / per)));

    // set output from click position (no toggle)
    const output: WaveOutput = y < height / 2 ? 'high' : 'low';
    const p: WavePoint = { tick, output };

    // insert sorted; replace if duplicate tick exists (previous or next)
    const next: WavePoint[] = [];
    let inserted = false;
    for (let i = 0; i < value.changeAt.length; i++) {
      const c = value.changeAt[i];
      if (!inserted && tick <= c.tick) {
        if (tick === c.tick) {
          next.push(p);
          inserted = true;
          // skip existing at same tick
          continue;
        } else {
          // check if equals the previous we just pushed
          const last = next[next.length - 1];
          if (last && last.tick === tick) {
            next[next.length - 1] = p; // replace previous duplicate
          } else {
            next.push(p);
          }
          inserted = true;
        }
      }
      next.push(c);
    }
    if (!inserted) {
      // handle potential duplicate with last
      if (next.length > 0 && next[next.length - 1].tick === tick) next[next.length - 1] = p;
      else next.push(p);
    }

    commit(normalizeTicks(next, value.totalTicks));
  }

  function undo() {
    if (past.length === 0) return;
    const prev = past[past.length - 1];
    setPast(p => p.slice(0, -1));
    setFuture(f => [value, ...f]);
    onChange(prev);
  }
  function redo() {
    if (future.length === 0) return;
    const next = future[0];
    setFuture(f => f.slice(1));
    setPast(p => [...p, value]);
    onChange(next);
  }

  function onKeyDown(e: React.KeyboardEvent<HTMLDivElement>) {
    if (e.ctrlKey && e.key.toLowerCase() === 'z') {
      if (e.shiftKey) redo();
      else undo();
      e.preventDefault();
    } else if (e.ctrlKey && e.key.toLowerCase() === 'y') {
      redo();
      e.preventDefault();
    }
  }

  function onMarkerPointerMove(e: React.PointerEvent<SVGGElement>) {
    // delegate to svg-level handler to keep logic centralized
    onPointerMove(e as unknown as React.PointerEvent<SVGSVGElement>);
  }
  function onMarkerPointerUp(e: React.PointerEvent<SVGGElement>) {
    onPointerUp(e as unknown as React.PointerEvent<SVGSVGElement>);
  }

  return (
    <div className="space-y-2" onKeyDown={onKeyDown} tabIndex={0}>
      {(() => {
        const view: Waveform = dragPreview ? { ...value, changeAt: dragPreview } : value;
        const segs = toSegments(view);
        return (
          <svg
            ref={svgRef}
            width="100%"
            height={height}
            onClick={onSvgClick}
            onPointerMove={onPointerMove}
            onPointerUp={onPointerUp}
            className="bg-slate-900/60 rounded border border-slate-700/50"
            style={{ touchAction: 'none' }}
          >
            {/* grid */}
            <g>
              <line x1={12} y1={height / 2} x2="100%" y2={height / 2} stroke="#334155" strokeDasharray="4 4" />
            </g>

            {/* waveform */}
            <polyline
              fill="none"
              stroke="#22c55e"
              strokeWidth="2"
              points={segs
                .flatMap(s => [
                  [12 + s.from * pxPerTick(svgRef.current?.clientWidth ?? 600), yFor(s.output)],
                  [12 + s.to * pxPerTick(svgRef.current?.clientWidth ?? 600), yFor(s.output)],
                ])
                .map(p => p.join(','))
                .join(' ')}
            />

            {/* markers */}
            {view.changeAt.map((p, i) => {
              const x = 12 + p.tick * pxPerTick(svgRef.current?.clientWidth ?? 600);
              const y = yFor(p.output);
              return (
                <g
                  key={i}
                  onPointerDown={(e) => onPointerDown(e, i)}
                  onPointerMove={onMarkerPointerMove}
                  onPointerUp={onMarkerPointerUp}
                  style={{ cursor: 'grab' }}
                >
                  <circle cx={x} cy={y} r={12} fill="#0b1220" stroke="#94a3b8" strokeWidth={2} />
                  <text x={x} y={y + 4} fontSize="12" textAnchor="middle" fill="#e2e8f0">
                    {i}
                  </text>
                </g>
              );
            })}
          </svg>
        );
      })()}

      {/* hint */}
      <div className="text-xs text-slate-400">
        Tip: Click the timeline to add a marker at that tick (above the center line = high, below = low). Drag markers to move. Use Ctrl+Z / Ctrl+Shift+Z to undo/redo.
      </div>

      <div className="flex flex-wrap gap-2 items-center">
        <button className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white" onClick={undo} disabled={past.length === 0}>
          Undo
        </button>
        <button className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white" onClick={redo} disabled={future.length === 0}>
          Redo
        </button>
      </div>

      <div>
        <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">Json Config</div>
        <textarea
          className="w-full h-48 bg-slate-950/80 rounded border border-slate-700/50 p-2 font-mono text-sm"
          value={JSON.stringify(value, null, 2)}
          onChange={e => {
            try {
              const parsed = JSON.parse(e.target.value);
              const res = zWaveform.safeParse(parsed);
              if (res.success) onChange(res.data);
            } catch {
              // ignore invalid json while typing
            }
          }}
        />
      </div>
    </div>
  );
}
