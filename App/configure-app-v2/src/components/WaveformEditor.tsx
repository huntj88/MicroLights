import { useMemo, useRef, useState, useEffect } from 'react';
import { toast } from 'sonner';

import { zWaveform, type Waveform, type WavePoint, toSegments, toggle, type WaveOutput } from '@/lib/waveform';

type Props = {
  value: Waveform;
  onChange: (wf: Waveform) => void;
  height?: number;
};

// Simple square-wave timeline editor with draggable markers
export function WaveformEditor({ value, onChange, height = 160 }: Props) {
  const [dragIndex, setDragIndex] = useState<number | null>(null);
  const svgRef = useRef<SVGSVGElement>(null);

  // history for undo/redo
  const [past, setPast] = useState<Waveform[]>([]);
  const [future, setFuture] = useState<Waveform[]>([]);

  useEffect(() => {
    setFuture([]);
  }, [value.name, value.totalTicks, value.changeAt]);

  const segs = useMemo(() => toSegments(value), [value]);

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

  function addMarker() {
    const last = value.changeAt[value.changeAt.length - 1];
    const nextTick = Math.min(value.totalTicks - 1, last.tick + Math.max(1, Math.floor(value.totalTicks / 6)));
    const p: WavePoint = { tick: nextTick, output: toggle(last.output) };
    commit([...value.changeAt, p]);
  }

  function removeMarker() {
    if (value.changeAt.length <= 1) return;
    commit(value.changeAt.slice(0, -1));
  }

  function onPointerDown(idx: number) {
    setDragIndex(idx);
  }

  function onPointerMove(e: React.PointerEvent<SVGSVGElement>) {
    if (dragIndex == null || !svgRef.current) return;
    const bounds = svgRef.current.getBoundingClientRect();
    const width = bounds.width;
    const x = Math.min(Math.max(e.clientX - bounds.left, 12), width - 12);
    const per = pxPerTick(width);
    const tick = Math.round((x - 12) / per);

    const y = e.clientY - bounds.top;
    const output: WaveOutput = y < height / 2 ? 'high' : 'low';

    const next: WavePoint[] = value.changeAt.map((p, i) => (i === dragIndex ? { tick, output } : p));
    // keep strictly increasing ticks
    for (let i = 1; i < next.length; i++) {
      if (next[i].tick <= next[i - 1].tick) next[i].tick = next[i - 1].tick + 1;
    }
    // clamp to range
    for (let i = 0; i < next.length; i++) {
      next[i].tick = Math.max(0, Math.min(value.totalTicks - 1, next[i].tick));
    }
    onChange({ ...value, changeAt: next });
  }

  function onPointerUp() {
    if (dragIndex == null) return;
    setDragIndex(null);
    commit(value.changeAt);
  }

  function currentOutputAtTick(tick: number): WaveOutput {
    let state: WaveOutput = value.changeAt[0].output;
    for (const p of value.changeAt) {
      if (p.tick <= tick) state = p.output as WaveOutput;
      else break;
    }
    return state;
  }

  function onSvgClick(e: React.MouseEvent<SVGSVGElement>) {
    if (!svgRef.current) return;
    if (dragIndex != null) return; // ignore if currently dragging
    const bounds = svgRef.current.getBoundingClientRect();
    const width = bounds.width;
    const x = Math.min(Math.max(e.clientX - bounds.left, 12), width - 12);
    const per = pxPerTick(width);
    const tick = Math.max(0, Math.min(value.totalTicks - 1, Math.round((x - 12) / per)));

    const base = currentOutputAtTick(tick);
    const p: WavePoint = { tick, output: toggle(base) };

    // insert sorted; replace if duplicate tick exists
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
          next.push(p);
          inserted = true;
        }
      }
      next.push(c);
    }
    if (!inserted) next.push(p);

    commit(next);
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

  return (
    <div className="space-y-2" onKeyDown={onKeyDown} tabIndex={0}>
      <svg
        ref={svgRef}
        width="100%"
        height={height}
        onClick={onSvgClick}
        onPointerMove={onPointerMove}
        onPointerUp={onPointerUp}
        className="bg-slate-900/60 rounded border border-slate-700/50"
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
        {value.changeAt.map((p, i) => {
          const x = 12 + p.tick * pxPerTick(svgRef.current?.clientWidth ?? 600);
          const y = yFor(p.output);
          return (
            <g key={i} onPointerDown={() => onPointerDown(i)} style={{ cursor: 'grab' }}>
              <circle cx={x} cy={y} r={12} fill="#0b1220" stroke="#94a3b8" strokeWidth={2} />
              <text x={x} y={y + 4} fontSize="12" textAnchor="middle" fill="#e2e8f0">
                {i}
              </text>
            </g>
          );
        })}
      </svg>

      <div className="flex flex-wrap gap-2 items-center">
        <button className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white" onClick={addMarker}>
          Add Marker to end
        </button>
        <button className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white" onClick={removeMarker}>
          Remove Marker from end
        </button>
        <button className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white" onClick={undo} disabled={past.length === 0}>
          Undo
        </button>
        <button className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white" onClick={redo} disabled={future.length === 0}>
          Redo
        </button>
        <button
          className="ml-auto px-3 py-1.5 rounded bg-fg-ring/80 hover:bg-fg-ring text-slate-900"
          onClick={() => toast.success('Waveform saved')}
        >
          Save Wave Form
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
