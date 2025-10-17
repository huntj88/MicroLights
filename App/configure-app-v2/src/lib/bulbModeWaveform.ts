import { z } from 'zod';

export type BulbModeWaveOutput = 'high' | 'low';
export type BulbModeWavePoint = { tick: number; output: BulbModeWaveOutput };
export type BulbModeWaveform = {
  name: string;
  totalTicks: number; // total discrete ticks in the cycle
  changeAt: BulbModeWavePoint[]; // sorted by tick asc, must start at tick 0
};

export const zBulbModeWavePoint = z.object({
  tick: z.number().int().min(0),
  output: z.union([z.literal('high'), z.literal('low')]),
});

export const zBulbModeWaveform = z
  .object({
    name: z.string().min(1),
    totalTicks: z.number().int().min(2),
    changeAt: z.array(zBulbModeWavePoint),
  })
  .superRefine((wf, ctx) => {
    if (wf.changeAt.length === 0) {
      ctx.addIssue({ code: 'custom', message: 'changeAt must have at least one point' });
      return;
    }
    // must start at 0
    if (wf.changeAt[0].tick !== 0) {
      ctx.addIssue({ code: 'custom', message: 'first change must be at tick 0' });
    }
    // strictly increasing ticks and within totalTicks
    for (let i = 0; i < wf.changeAt.length; i++) {
      const p = wf.changeAt[i];
      if (p.tick < 0 || p.tick >= wf.totalTicks) {
        ctx.addIssue({ code: 'custom', message: `tick ${p.tick} out of range` });
        break;
      }
      if (i > 0 && p.tick <= wf.changeAt[i - 1].tick) {
        ctx.addIssue({ code: 'custom', message: 'ticks must be strictly increasing' });
        break;
      }
    }
  });

export type BulbModeWaveSegment = { from: number; to: number; output: BulbModeWaveOutput };

export function bulbModeToSegments(wf: BulbModeWaveform): BulbModeWaveSegment[] {
  const segs: BulbModeWaveSegment[] = [];
  for (let i = 0; i < wf.changeAt.length; i++) {
    const curr = wf.changeAt[i];
    const nextTick = i + 1 < wf.changeAt.length ? wf.changeAt[i + 1].tick : wf.totalTicks;
    segs.push({ from: curr.tick, to: nextTick, output: curr.output });
  }
  return segs;
}

export function toggleBulbModeOutput(output: BulbModeWaveOutput): BulbModeWaveOutput {
  return output === 'high' ? 'low' : 'high';
}
