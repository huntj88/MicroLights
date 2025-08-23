import { z } from 'zod';

export type WaveOutput = 'high' | 'low';
export type WavePoint = { tick: number; output: WaveOutput };
export type Waveform = {
  name: string;
  totalTicks: number; // total discrete ticks in the cycle
  changeAt: WavePoint[]; // sorted by tick asc, must start at tick 0
};

export const zWavePoint = z.object({
  tick: z.number().int().min(0),
  output: z.union([z.literal('high'), z.literal('low')]),
});

export const zWaveform = z
  .object({
    name: z.string().min(1),
    totalTicks: z.number().int().min(2),
    changeAt: z.array(zWavePoint),
  })
  .superRefine((wf, ctx) => {
    if (wf.changeAt.length === 0) {
      ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'changeAt must have at least one point' });
      return;
    }
    // must start at 0
    if (wf.changeAt[0].tick !== 0) {
      ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'first change must be at tick 0' });
    }
    // strictly increasing ticks and within totalTicks
    for (let i = 0; i < wf.changeAt.length; i++) {
      const p = wf.changeAt[i];
      if (p.tick < 0 || p.tick >= wf.totalTicks) {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: `tick ${p.tick} out of range` });
        break;
      }
      if (i > 0 && p.tick <= wf.changeAt[i - 1].tick) {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: 'ticks must be strictly increasing' });
        break;
      }
    }
  });

export type WaveSegment = { from: number; to: number; output: WaveOutput };

export function toSegments(wf: Waveform): WaveSegment[] {
  const segs: WaveSegment[] = [];
  for (let i = 0; i < wf.changeAt.length; i++) {
    const curr = wf.changeAt[i];
    const nextTick = i + 1 < wf.changeAt.length ? wf.changeAt[i + 1].tick : wf.totalTicks;
    segs.push({ from: curr.tick, to: nextTick, output: curr.output });
  }
  return segs;
}

export function toggle(output: WaveOutput): WaveOutput {
  return output === 'high' ? 'low' : 'high';
}
