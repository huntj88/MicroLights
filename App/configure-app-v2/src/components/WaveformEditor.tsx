import { useRef, useState, useEffect } from 'react';
import { useTranslation } from 'react-i18next';
import { toast } from 'sonner';

import {
  zWaveform,
  type Waveform,
  type WavePoint,
  toSegments,
  type WaveOutput,
} from '@/lib/waveform';

type Props = {
  value: Waveform;
  onChange: (wf: Waveform) => void;
  height?: number;
  readOnly?: boolean;
};

// Simple square-wave timeline editor with draggable markers
export function WaveformEditor({ value, onChange, height = 160, readOnly = false }: Props) {
  const { t } = useTranslation();
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
      toast.error(res.error.issues[0]?.message ?? t('invalidWaveform'));
      return;
    }
    pushHistory(value);
    onChange(res.data);
  }

  function onPointerDown(e: React.PointerEvent<SVGGElement>, idx: number) {
    if (readOnly) return;
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

  // helper to clamp a tick for the dragged index between its neighbors
  function clampTickForIndex(
    points: WavePoint[],
    idx: number,
    tick: number,
    totalTicks: number,
  ): number {
    if (idx === 0) return 0; // first marker must stay at 0
    const prevTick = points[idx - 1]?.tick ?? 0;
    const nextTick = points[idx + 1]?.tick ?? totalTicks; // if last, allow up to totalTicks - 1
    const min = prevTick + 1;
    const max = idx === points.length - 1 ? totalTicks - 1 : nextTick - 1;
    return Math.max(min, Math.min(max, tick));
  }

  function onPointerMove(e: React.PointerEvent<SVGSVGElement>) {
    if (readOnly || dragIndex == null || !svgRef.current) return;
    e.preventDefault();
    suppressNextClickRef.current = true; // mark that a drag occurred
    const bounds = svgRef.current.getBoundingClientRect();
    const width = bounds.width;
    const x = Math.min(Math.max(e.clientX - bounds.left, 12), width - 12);
    const per = pxPerTick(width);
    let tick = Math.round((x - 12) / per);

    const y = e.clientY - bounds.top;
    const output: WaveOutput = y < height / 2 ? 'high' : 'low';

    // clamp the dragged point between neighbors so others do not move
    tick = clampTickForIndex(value.changeAt, dragIndex, tick, value.totalTicks);

    const next: WavePoint[] = value.changeAt.map((p, i) =>
      i === dragIndex ? { tick, output } : p,
    );

    // update local preview only; commit on pointer up
    setDragPreview(next);
  }

  function onPointerUp(e: React.PointerEvent<SVGSVGElement>) {
    if (readOnly) return;
    e.stopPropagation();
    if (dragIndex == null) return;
    const points = dragPreview ?? value.changeAt;
    setDragPreview(null);
    setDragIndex(null);
    suppressNextClickRef.current = true;
    commit(points);
  }

  function onSvgClick(e: React.MouseEvent<SVGSVGElement>) {
    if (readOnly) return;
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

    commit(next);
  }

  function undo() {
    if (readOnly) return;
    if (past.length === 0) return;
    const prev = past[past.length - 1];
    setPast(p => p.slice(0, -1));
    setFuture(f => [value, ...f]);
    onChange(prev);
  }
  function redo() {
    if (readOnly) return;
    if (future.length === 0) return;
    const next = future[0];
    setFuture(f => f.slice(1));
    setPast(p => [...p, value]);
    onChange(next);
  }

  function onKeyDown(e: React.KeyboardEvent<HTMLDivElement>) {
    if (readOnly) return;
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

  function removeMarker(index: number) {
    if (readOnly) return;
    if (index === 0) {
      toast.error(t('cannotDeleteFirstMarker'));
      return;
    }
    const next = value.changeAt.filter((_, i) => i !== index);
    pushHistory(value);
    onChange({ ...value, changeAt: next });
    suppressNextClickRef.current = true;
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
              <line
                x1={12}
                y1={height / 2}
                x2="100%"
                y2={height / 2}
                stroke="#334155"
                strokeDasharray="4 4"
              />
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
                  onPointerDown={e => onPointerDown(e, i)}
                  onPointerMove={onMarkerPointerMove}
                  onPointerUp={onMarkerPointerUp}
                  onDoubleClick={e => {
                    e.stopPropagation();
                    e.preventDefault();
                    removeMarker(i);
                  }}
                  style={{
                    cursor: readOnly ? 'default' : 'grab',
                    pointerEvents: readOnly ? 'none' : 'auto',
                  }}
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
      <div className="text-xs text-slate-400">{t('tipWaveformEditor')}</div>

      <div className="flex flex-wrap gap-2 items-center">
        <button
          className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white"
          onClick={undo}
          disabled={readOnly || past.length === 0}
        >
          {t('undo')}
        </button>
        <button
          className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white"
          onClick={redo}
          disabled={readOnly || future.length === 0}
        >
          {t('redo')}
        </button>
      </div>

      <div>
        <div className="text-xs uppercase tracking-wide text-slate-400 mb-1">{t('jsonConfig')}</div>
        <textarea
          className="w-full h-48 bg-slate-950/80 rounded border border-slate-700/50 p-2 font-mono text-sm"
          value={JSON.stringify(value, null, 2)}
          disabled={readOnly}
          onChange={e => {
            if (readOnly) return;
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
