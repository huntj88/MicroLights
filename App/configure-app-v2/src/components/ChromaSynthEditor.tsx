import clsx from 'clsx';
import { useCallback, useEffect, useMemo, useRef, useState } from 'react';

import type {
  ChromaChannelKey,
  ChromaSynthChannelState,
  ChromaSynthSection,
  ChromaSynthState,
  ChromaSynthFrame,
  ChromaSynthExportFormat,
} from '@/types/chromaSynth';

const CHANNELS: Array<{ key: ChromaChannelKey; label: string; color: string }> = [
  { key: 'red', label: 'Red Channel', color: '#ef4444' },
  { key: 'green', label: 'Green Channel', color: '#22c55e' },
  { key: 'blue', label: 'Blue Channel', color: '#3b82f6' },
];

const AXIS_MARKS = [0, 64, 128, 192, 255];
const DEFAULT_SECTION_DURATION = 1000;
const DEFAULT_EQUATION = '0';
const DEFAULT_PREVIEW_SAMPLES = 120;
const DEFAULT_WAVEFORM_SAMPLES = 240;

type EquationContext = {
  timeMs: number;
  sectionTimeMs: number;
  durationMs: number;
  progress: number;
  index: number;
};

type EquationEvaluator = (context: EquationContext) => number;

type TimelineSegment = {
  section: ChromaSynthSection;
  startMs: number;
  endMs: number;
  evaluator: EquationEvaluator;
  index: number;
};

type TimelineMap = Record<ChromaChannelKey, TimelineSegment[]>;

type SectionDraft = {
  label: string;
  durationMs: number;
  equation: string;
};

type Props = {
  value: ChromaSynthState;
  onChange: (next: ChromaSynthState) => void;
  readOnly?: boolean;
  previewSamples?: number;
  waveformSamples?: number;
  onExport?: (format: ChromaSynthExportFormat, frames: ChromaSynthFrame[]) => void;
};

export function ChromaSynthEditor({
  value,
  onChange,
  readOnly = false,
  previewSamples = DEFAULT_PREVIEW_SAMPLES,
  waveformSamples = DEFAULT_WAVEFORM_SAMPLES,
  onExport,
}: Props) {
  const normalized = useMemo(() => normalizeState(value), [value]);
  const timelines = useMemo(() => buildTimelines(normalized), [normalized]);
  const totalDurationMs = useMemo(
    () => Math.max(0, ...CHANNELS.map(channel => getTimelineDuration(timelines[channel.key]))),
    [timelines],
  );

  const frames = useMemo(
    () => buildPreviewFrames(timelines, totalDurationMs, previewSamples),
    [timelines, totalDurationMs, previewSamples],
  );

  const gradientStyle = useMemo(() => buildGradient(frames), [frames]);

  const [playheadMs, setPlayheadMs] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const animationRef = useRef<number | null>(null);
  const startRef = useRef<number>(0);

  useEffect(() => {
    if (totalDurationMs <= 0) {
      setPlayheadMs(0);
      setIsPlaying(false);
      return;
    }
    if (playheadMs > totalDurationMs) setPlayheadMs(totalDurationMs);
  }, [totalDurationMs, playheadMs]);

  useEffect(() => {
    if (!isPlaying || totalDurationMs <= 0) return;

    const begin = performance.now() - playheadMs;
    startRef.current = begin;

    const tick = (now: number) => {
      const elapsed = now - startRef.current;
      if (elapsed >= totalDurationMs) {
        setPlayheadMs(totalDurationMs);
        setIsPlaying(false);
        animationRef.current = null;
        return;
      }
      setPlayheadMs(elapsed);
      animationRef.current = requestAnimationFrame(tick);
    };

    animationRef.current = requestAnimationFrame(tick);

    return () => {
      if (animationRef.current != null) {
        cancelAnimationFrame(animationRef.current);
        animationRef.current = null;
      }
    };
  }, [isPlaying, totalDurationMs]);

  const currentColor = useMemo(() => {
    const red = evaluateTimeline(timelines.red, playheadMs);
    const green = evaluateTimeline(timelines.green, playheadMs);
    const blue = evaluateTimeline(timelines.blue, playheadMs);
    return { red, green, blue };
  }, [timelines, playheadMs]);

  const updateChannel = useCallback(
    (channel: ChromaChannelKey, sections: ChromaSynthSection[]) => {
      if (readOnly) return;
      const next: ChromaSynthState = {
        red: cloneChannel(normalized.red),
        green: cloneChannel(normalized.green),
        blue: cloneChannel(normalized.blue),
      };
      next[channel] = { sections };
      onChange(next);
    },
    [normalized, onChange, readOnly],
  );

  const resetPlayback = useCallback(() => {
    setIsPlaying(false);
    setPlayheadMs(0);
  }, []);

  const handleExport = useCallback(
    (format: ChromaSynthExportFormat) => {
      if (!onExport) {
        console.warn('ChromaSynthEditor: onExport handler not provided');
        return;
      }
      onExport(format, frames);
    },
    [frames, onExport],
  );

  return (
    <div className="space-y-8">
      <section className="space-y-3">
        <header className="flex items-center justify-between">
          <h2 className="text-lg font-semibold text-slate-100">Combined Color Preview</h2>
          <div className="text-sm text-slate-400">
            Total Duration: {Math.round(totalDurationMs)} ms
          </div>
        </header>
        <div className="relative h-24 rounded-lg border border-slate-700 bg-slate-900 overflow-hidden">
          <div className="absolute inset-0" style={{ background: gradientStyle }} />
          <div className="absolute inset-0 pointer-events-none flex flex-col justify-between p-3">
            <div className="flex items-center justify-between text-xs text-slate-300">
              <span>Start</span>
              <span>End</span>
            </div>
            <div className="text-xs text-slate-300 self-end">{Math.round(playheadMs)} ms</div>
          </div>
          {totalDurationMs > 0 ? (
            <div
              className="absolute top-0 bottom-0 w-0.5 bg-white/70"
              style={{ left: `${(playheadMs / totalDurationMs) * 100}%` }}
            />
          ) : null}
        </div>
        <div className="flex items-center gap-4">
          <input
            className="flex-1"
            type="range"
            min={0}
            max={Math.max(1, Math.round(totalDurationMs))}
            step={1}
            value={Math.min(playheadMs, totalDurationMs)}
            onChange={event => {
              const next = Number(event.target.value);
              setPlayheadMs(next);
              setIsPlaying(false);
            }}
          />
          <div
            className="w-10 h-10 rounded border border-slate-600"
            style={{
              backgroundColor: `rgb(${currentColor.red}, ${currentColor.green}, ${currentColor.blue})`,
            }}
            aria-label="Current color"
          />
        </div>
        <div className="flex flex-wrap gap-3">
          <button
            type="button"
            className={clsx(
              'px-3 py-1.5 rounded bg-slate-800 text-slate-100 shadow-sm border border-slate-700 transition-colors',
              isPlaying ? 'hover:bg-slate-700' : 'hover:bg-slate-700',
              readOnly && 'opacity-50 cursor-not-allowed',
            )}
            onClick={() => {
              if (totalDurationMs <= 0) return;
              setIsPlaying(prev => !prev);
            }}
            disabled={totalDurationMs <= 0}
          >
            {isPlaying ? 'Pause' : 'Play'}
          </button>
          <button
            type="button"
            className="px-3 py-1.5 rounded bg-slate-800 text-slate-100 shadow-sm border border-slate-700 hover:bg-slate-700 transition-colors"
            onClick={resetPlayback}
          >
            Reset
          </button>
          <div className="flex items-center gap-2 text-sm text-slate-400">
            <span>Exports:</span>
            {(['mp4', 'gif', 'png-sequence'] as ChromaSynthExportFormat[]).map(format => (
              <button
                key={format}
                type="button"
                className="px-2.5 py-1 rounded bg-slate-900 border border-slate-700 text-slate-200 hover:bg-slate-800 transition-colors"
                onClick={() => handleExport(format)}
              >
                {format.toUpperCase()}
              </button>
            ))}
          </div>
        </div>
      </section>

      <section className="space-y-10">
        {CHANNELS.map(channel => (
          <ChannelLane
            key={channel.key}
            label={channel.label}
            accentColor={channel.color}
            sections={normalized[channel.key].sections}
            timeline={timelines[channel.key]}
            totalDurationMs={totalDurationMs}
            playheadMs={playheadMs}
            readOnly={readOnly}
            waveformSamples={waveformSamples}
            onSectionsChange={sections => updateChannel(channel.key, sections)}
          />
        ))}
      </section>
    </div>
  );
}

function ChannelLane({
  label,
  accentColor,
  sections,
  timeline,
  totalDurationMs,
  playheadMs,
  readOnly,
  waveformSamples,
  onSectionsChange,
}: {
  label: string;
  accentColor: string;
  sections: ChromaSynthSection[];
  timeline: TimelineSegment[];
  totalDurationMs: number;
  playheadMs: number;
  readOnly: boolean;
  waveformSamples: number;
  onSectionsChange: (next: ChromaSynthSection[]) => void;
}) {
  const { points, duration } = useMemo(
    () => sampleWaveform(timeline, totalDurationMs, waveformSamples),
    [timeline, totalDurationMs, waveformSamples],
  );

  const [expandedSection, setExpandedSection] = useState<string | null>(null);
  const [drafts, setDrafts] = useState<Record<string, SectionDraft>>({});

  useEffect(() => {
    setDrafts(prev => {
      const next: Record<string, SectionDraft> = {};
      for (const section of sections) {
        next[section.id] = prev[section.id] ?? toDraft(section);
      }
      return next;
    });
  }, [sections]);

  const ensureDraft = useCallback((section: ChromaSynthSection) => {
    setDrafts(prev => ({
      ...prev,
      [section.id]: prev[section.id] ?? toDraft(section),
    }));
  }, []);

  const viewBoxWidth = Math.max(1, duration);
  const pointCount = points.length;
  const toPoint = useCallback(
    (value: number, index: number) => {
      const x = (viewBoxWidth / Math.max(1, pointCount - 1)) * index;
      const y = 255 - value;
      return `${x.toFixed(2)},${y.toFixed(2)}`;
    },
    [pointCount, viewBoxWidth],
  );

  const playheadX = Math.min(viewBoxWidth, Math.max(0, playheadMs));

  return (
    <article className="space-y-4">
      <header className="flex flex-wrap items-baseline justify-between gap-3">
        <div className="flex items-center gap-2">
          <span
            className="inline-block h-3 w-3 rounded-full"
            style={{ backgroundColor: accentColor }}
          />
          <h3 className="text-base font-semibold text-slate-100">{label}</h3>
        </div>
        <div className="text-xs uppercase tracking-wide text-slate-400">
          {sections.length} section{sections.length === 1 ? '' : 's'}
        </div>
      </header>

      <div className="rounded-lg border border-slate-700 bg-slate-900/60 p-3">
        <svg
          className="w-full"
          viewBox={`0 0 ${viewBoxWidth} 255`}
          preserveAspectRatio="none"
          height={160}
        >
          {/* grid lines */}
          {AXIS_MARKS.map(mark => (
            <g key={mark}>
              <line
                x1={0}
                y1={255 - mark}
                x2={viewBoxWidth}
                y2={255 - mark}
                stroke="rgba(148, 163, 184, 0.2)"
                strokeDasharray="4 4"
              />
              <text x={4} y={255 - mark - 4} fontSize={10} fill="rgba(148, 163, 184, 0.8)">
                {mark}
              </text>
            </g>
          ))}

          <polyline
            fill="none"
            stroke={accentColor}
            strokeWidth={2}
            points={points.map(toPoint).join(' ')}
          />

          <line
            x1={playheadX}
            y1={0}
            x2={playheadX}
            y2={255}
            stroke="white"
            strokeWidth={1}
            strokeDasharray="6 4"
          />
        </svg>
        <div className="mt-2 flex justify-between text-xs text-slate-400">
          <span>0 ms</span>
          <span>{Math.round(viewBoxWidth)} ms</span>
        </div>
      </div>

      <div className="space-y-3">
        {sections.map((section, index) => {
          const draft = drafts[section.id] ?? toDraft(section);
          const isOpen = expandedSection === section.id;
          return (
            <div
              key={section.id}
              className="rounded-lg border border-slate-700 bg-slate-900/70 shadow-sm"
            >
              <button
                type="button"
                className="w-full flex items-center justify-between gap-3 px-4 py-3 text-left hover:bg-slate-800/60 transition-colors"
                onClick={() => {
                  ensureDraft(section);
                  setExpandedSection(prev => (prev === section.id ? null : section.id));
                }}
              >
                <div>
                  <div className="text-sm font-medium text-slate-100 flex items-center gap-2">
                    <span className="rounded bg-slate-800 px-2 py-0.5 text-xs text-slate-300">
                      #{index + 1}
                    </span>
                    <span>{section.label || 'Untitled Section'}</span>
                  </div>
                  <div className="mt-1 text-xs text-slate-400">
                    Duration: {Math.round(section.durationMs)} ms · Equation: {section.equation}
                  </div>
                </div>
                <span className="text-xs text-slate-400">{isOpen ? 'Collapse' : 'Expand'}</span>
              </button>

              {isOpen ? (
                <div className="border-t border-slate-700 px-4 py-3 space-y-3 bg-slate-900">
                  <div className="grid gap-3 md:grid-cols-2">
                    <label className="space-y-1 text-sm text-slate-300">
                      <span>Label</span>
                      <input
                        type="text"
                        className="w-full rounded border border-slate-700 bg-slate-950 px-2 py-1 text-slate-100"
                        value={draft.label}
                        onChange={event =>
                          setDrafts(prev => ({
                            ...prev,
                            [section.id]: { ...prev[section.id], label: event.target.value },
                          }))
                        }
                        disabled={readOnly}
                      />
                    </label>
                    <label className="space-y-1 text-sm text-slate-300">
                      <span>Duration (ms)</span>
                      <input
                        type="number"
                        min={1}
                        className="w-full rounded border border-slate-700 bg-slate-950 px-2 py-1 text-slate-100"
                        value={draft.durationMs}
                        onChange={event => {
                          const value = Math.max(1, Number(event.target.value) || 1);
                          setDrafts(prev => ({
                            ...prev,
                            [section.id]: { ...prev[section.id], durationMs: value },
                          }));
                        }}
                        disabled={readOnly}
                      />
                    </label>
                  </div>
                  <label className="space-y-1 text-sm text-slate-300">
                    <span>Equation (output will be clamped 0-255)</span>
                    <textarea
                      className="w-full min-h-[96px] rounded border border-slate-700 bg-slate-950 px-2 py-1 text-slate-100 font-mono text-sm"
                      value={draft.equation}
                      onChange={event =>
                        setDrafts(prev => ({
                          ...prev,
                          [section.id]: { ...prev[section.id], equation: event.target.value },
                        }))
                      }
                      disabled={readOnly}
                    />
                  </label>
                  <div className="text-xs text-slate-400">
                    Variables: <code>t</code> (ms), <code>sectionTime</code> (ms),{' '}
                    <code>duration</code> (ms),
                    <code>progress</code> (0-1), <code>sectionIndex</code> (0-based)
                  </div>
                  <div className="flex flex-wrap gap-2">
                    <button
                      type="button"
                      className="px-3 py-1.5 rounded bg-emerald-600 text-white hover:bg-emerald-500 transition-colors"
                      onClick={() => {
                        if (readOnly) return;
                        const sanitized = {
                          label: draft.label.trim(),
                          durationMs: Math.max(1, draft.durationMs),
                          equation: draft.equation.trim() || DEFAULT_EQUATION,
                        };
                        onSectionsChange(
                          sections.map(current =>
                            current.id === section.id ? { ...current, ...sanitized } : current,
                          ),
                        );
                        setExpandedSection(null);
                      }}
                      disabled={readOnly}
                    >
                      Save changes
                    </button>
                    <button
                      type="button"
                      className="px-3 py-1.5 rounded bg-slate-800 text-slate-200 hover:bg-slate-700 transition-colors"
                      onClick={() => {
                        setDrafts(prev => ({
                          ...prev,
                          [section.id]: toDraft(section),
                        }));
                      }}
                    >
                      Reset
                    </button>
                  </div>
                </div>
              ) : null}

              <div className="flex flex-wrap items-center justify-between gap-2 border-t border-slate-800 px-4 py-2 text-xs text-slate-400">
                <div className="flex items-center gap-2">
                  <button
                    type="button"
                    className="rounded border border-slate-700 px-2 py-1 text-slate-200 hover:bg-slate-800 transition-colors disabled:opacity-50"
                    onClick={() => moveSection(sections, section.id, -1, onSectionsChange)}
                    disabled={readOnly || index === 0}
                  >
                    Move Up
                  </button>
                  <button
                    type="button"
                    className="rounded border border-slate-700 px-2 py-1 text-slate-200 hover:bg-slate-800 transition-colors disabled:opacity-50"
                    onClick={() => moveSection(sections, section.id, 1, onSectionsChange)}
                    disabled={readOnly || index === sections.length - 1}
                  >
                    Move Down
                  </button>
                </div>
                <div className="flex items-center gap-2">
                  <button
                    type="button"
                    className="rounded border border-slate-700 px-2 py-1 text-slate-200 hover:bg-slate-800 transition-colors"
                    onClick={() => duplicateSection(section, sections, onSectionsChange)}
                    disabled={readOnly}
                  >
                    Duplicate
                  </button>
                  <button
                    type="button"
                    className="rounded border border-rose-700 px-2 py-1 text-rose-200 hover:bg-rose-900/40 transition-colors"
                    onClick={() => removeSection(section.id, sections, onSectionsChange)}
                    disabled={readOnly || sections.length === 1}
                  >
                    Delete
                  </button>
                </div>
              </div>
            </div>
          );
        })}

        <button
          type="button"
          className="inline-flex items-center gap-2 rounded border border-dashed border-slate-600 px-3 py-2 text-sm text-slate-200 hover:bg-slate-800/40"
          onClick={() => {
            if (readOnly) return;
            const newSection: ChromaSynthSection = {
              id: createSectionId(),
              label: `Section ${sections.length + 1}`,
              durationMs:
                sections.length > 0
                  ? (sections[sections.length - 1]?.durationMs ?? DEFAULT_SECTION_DURATION)
                  : DEFAULT_SECTION_DURATION,
              equation:
                sections.length > 0
                  ? (sections[sections.length - 1]?.equation ?? DEFAULT_EQUATION)
                  : DEFAULT_EQUATION,
            };
            const next = [...sections, newSection];
            onSectionsChange(next);
            setExpandedSection(newSection.id);
            ensureDraft(newSection);
          }}
          disabled={readOnly}
        >
          <span className="text-lg leading-none">＋</span>
          Add new section
        </button>
      </div>
    </article>
  );
}

function normalizeState(state: ChromaSynthState): ChromaSynthState {
  const safe = (channel: ChromaSynthChannelState | undefined): ChromaSynthChannelState => ({
    sections: channel?.sections ? [...channel.sections] : [],
  });
  return {
    red: safe(state.red),
    green: safe(state.green),
    blue: safe(state.blue),
  };
}

function cloneChannel(channel: ChromaSynthChannelState): ChromaSynthChannelState {
  return { sections: channel.sections.map(section => ({ ...section })) };
}

function toDraft(section: ChromaSynthSection): SectionDraft {
  return {
    label: section.label ?? '',
    durationMs: Math.max(1, section.durationMs),
    equation: section.equation ?? DEFAULT_EQUATION,
  };
}

function createSectionId(): string {
  if (typeof crypto !== 'undefined' && 'randomUUID' in crypto) {
    return crypto.randomUUID();
  }
  return Math.random().toString(36).slice(2);
}

function moveSection(
  sections: ChromaSynthSection[],
  id: string,
  delta: number,
  onSectionsChange: (next: ChromaSynthSection[]) => void,
) {
  const index = sections.findIndex(section => section.id === id);
  if (index < 0) return;
  const target = index + delta;
  if (target < 0 || target >= sections.length) return;
  const next = [...sections];
  const [current] = next.splice(index, 1);
  next.splice(target, 0, current);
  onSectionsChange(next);
}

function removeSection(
  id: string,
  sections: ChromaSynthSection[],
  onSectionsChange: (next: ChromaSynthSection[]) => void,
) {
  if (!window.confirm('Remove this section?')) return;
  const next = sections.filter(section => section.id !== id);
  onSectionsChange(next.length > 0 ? next : sections);
}

function duplicateSection(
  section: ChromaSynthSection,
  sections: ChromaSynthSection[],
  onSectionsChange: (next: ChromaSynthSection[]) => void,
) {
  const copy: ChromaSynthSection = {
    ...section,
    id: createSectionId(),
    label: `${section.label ?? 'Section'} copy`,
  };
  const index = sections.findIndex(candidate => candidate.id === section.id);
  const next = [...sections];
  next.splice(index + 1, 0, copy);
  onSectionsChange(next);
}

function buildTimelines(state: ChromaSynthState): TimelineMap {
  return {
    red: buildTimeline(state.red.sections),
    green: buildTimeline(state.green.sections),
    blue: buildTimeline(state.blue.sections),
  };
}

function buildTimeline(sections: ChromaSynthSection[]): TimelineSegment[] {
  const timeline: TimelineSegment[] = [];
  let cursor = 0;
  sections.forEach((section, index) => {
    const duration = Math.max(1, Number(section.durationMs) || DEFAULT_SECTION_DURATION);
    const evaluator = createEquationEvaluator(section);
    const segment: TimelineSegment = {
      section,
      startMs: cursor,
      endMs: cursor + duration,
      evaluator,
      index,
    };
    timeline.push(segment);
    cursor += duration;
  });
  return timeline;
}

function createEquationEvaluator(section: ChromaSynthSection): EquationEvaluator {
  const expression = (section.equation ?? DEFAULT_EQUATION).trim() || DEFAULT_EQUATION;
  try {
    const fn = new Function(
      't',
      'sectionTime',
      'duration',
      'progress',
      'sectionIndex',
      'Math',
      `return (${expression});`,
    ) as (
      t: number,
      sectionTime: number,
      duration: number,
      progress: number,
      sectionIndex: number,
      math: Math,
    ) => number;

    return ({ timeMs, sectionTimeMs, durationMs, progress, index: ctxIndex }) => {
      try {
        const result = fn(timeMs, sectionTimeMs, durationMs, progress, ctxIndex, Math);
        return Number.isFinite(result) ? result : Number.NaN;
      } catch {
        return Number.NaN;
      }
    };
  } catch {
    return () => Number.NaN;
  }
}

function evaluateTimeline(timeline: TimelineSegment[], timeMs: number): number {
  if (timeline.length === 0) return 0;
  const last = timeline[timeline.length - 1];
  if (timeMs >= last.endMs) {
    const duration = Math.max(1, last.endMs - last.startMs);
    const value = last.evaluator({
      timeMs,
      sectionTimeMs: duration,
      durationMs: duration,
      progress: 1,
      index: last.index,
    });
    return clampColor(value);
  }
  for (const segment of timeline) {
    if (timeMs <= segment.endMs) {
      const duration = Math.max(1, segment.endMs - segment.startMs);
      const sectionTime = Math.max(0, timeMs - segment.startMs);
      const progress = Math.min(1, sectionTime / duration);
      const value = segment.evaluator({
        timeMs,
        sectionTimeMs: sectionTime,
        durationMs: duration,
        progress,
        index: segment.index,
      });
      return clampColor(value);
    }
  }
  return 0;
}

function getTimelineDuration(timeline: TimelineSegment[]): number {
  if (timeline.length === 0) return 0;
  return timeline[timeline.length - 1]!.endMs;
}

function sampleWaveform(
  timeline: TimelineSegment[],
  totalDurationMs: number,
  sampleCount: number,
): { points: number[]; duration: number } {
  const count = Math.max(2, Math.floor(sampleCount));
  const duration = Math.max(totalDurationMs, getTimelineDuration(timeline), 1);
  const step = duration / (count - 1);
  const samples: number[] = [];
  for (let index = 0; index < count; index += 1) {
    const time = index === count - 1 ? duration : step * index;
    samples.push(clampColor(evaluateTimeline(timeline, time)));
  }
  return { points: samples, duration };
}

function buildPreviewFrames(
  timelines: TimelineMap,
  totalDurationMs: number,
  sampleCount: number,
): ChromaSynthFrame[] {
  const duration = Math.max(
    totalDurationMs,
    getTimelineDuration(timelines.red),
    getTimelineDuration(timelines.green),
    getTimelineDuration(timelines.blue),
  );
  if (duration <= 0) return [{ timeMs: 0, red: 0, green: 0, blue: 0 }];
  const count = Math.max(2, Math.floor(sampleCount));
  const step = duration / (count - 1);
  const frames: ChromaSynthFrame[] = [];
  for (let index = 0; index < count; index += 1) {
    const time = index === count - 1 ? duration : step * index;
    frames.push({
      timeMs: time,
      red: evaluateTimeline(timelines.red, time),
      green: evaluateTimeline(timelines.green, time),
      blue: evaluateTimeline(timelines.blue, time),
    });
  }
  return frames;
}

function buildGradient(frames: ChromaSynthFrame[]): string {
  if (frames.length === 0) return 'linear-gradient(90deg, #000000, #000000)';
  if (frames.length === 1) {
    const { red, green, blue } = frames[0];
    return `linear-gradient(90deg, rgb(${red}, ${green}, ${blue}), rgb(${red}, ${green}, ${blue}))`;
  }
  const stops = frames
    .map((frame, index) => {
      const percent = (index / Math.max(1, frames.length - 1)) * 100;
      return `rgb(${frame.red}, ${frame.green}, ${frame.blue}) ${percent.toFixed(2)}%`;
    })
    .join(', ');
  return `linear-gradient(90deg, ${stops})`;
}

function clampColor(value: number): number {
  if (!Number.isFinite(value)) return 0;
  return Math.max(0, Math.min(255, Math.round(value)));
}
