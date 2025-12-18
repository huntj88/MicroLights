import { z } from 'zod';

import { createLocalizedError } from '../../utils/localization';

const binaryOutputs = ['high', 'low'] as const;

export const binaryOutputSchema = z
  .enum(binaryOutputs)
  .describe('Binary output levels used by digital LEDs.');

export type BinaryOutput = z.infer<typeof binaryOutputSchema>;

const HEX_COLOR_REGEX = /^#(?:[0-9a-fA-F]{6})$/;

export const hexColorSchema = z
  .string()
  .regex(HEX_COLOR_REGEX, {
    message: 'validation.pattern.simple.hexColor',
  })
  .brand('HexColor');

export type HexColor = z.infer<typeof hexColorSchema>;

export const patternChangeBinarySchema = z.object({
  ms: z
    .number()
    .int('validation.pattern.simple.timestamp.integer')
    .min(0, 'validation.pattern.simple.timestamp.negative'),
  output: binaryOutputSchema,
});

export type BinaryPatternChange = z.infer<typeof patternChangeBinarySchema>;

export const patternChangeColorSchema = z.object({
  ms: z
    .number()
    .int('validation.pattern.simple.timestamp.integer')
    .min(0, 'validation.pattern.simple.timestamp.negative'),
  output: hexColorSchema,
});

export type ColorPatternChange = z.infer<typeof patternChangeColorSchema>;

export const patternChangeSchema = z.union([patternChangeBinarySchema, patternChangeColorSchema]);

export type PatternChange = z.infer<typeof patternChangeSchema>;

export const equationSectionSchema = z.object({
  equation: z.string().min(1, 'validation.pattern.equation.required'),
  duration: z.number().min(1, 'validation.pattern.duration.min'),
});

export type EquationSection = z.infer<typeof equationSectionSchema>;

export const channelConfigSchema = z.object({
  sections: z.array(equationSectionSchema),
  loopAfterDuration: z.boolean(),
});

export type ChannelConfig = z.infer<typeof channelConfigSchema>;

export const equationPatternSchema = z
  .object({
    type: z.literal('equation'),
    name: z.string().min(1, 'validation.pattern.name.required'),
    duration: z.number().nonnegative(),
    red: channelConfigSchema,
    green: channelConfigSchema,
    blue: channelConfigSchema,
  })
  .refine(
    data =>
      data.red.sections.length > 0 ||
      data.green.sections.length > 0 ||
      data.blue.sections.length > 0,
    {
      message: 'validation.pattern.equation.sectionRequired',
    },
  );

export type EquationPattern = z.infer<typeof equationPatternSchema>;

export const createDefaultEquationPattern = (): EquationPattern => ({
  type: 'equation',
  name: '',
  duration: 1000,
  red: { sections: [], loopAfterDuration: true },
  green: { sections: [], loopAfterDuration: true },
  blue: { sections: [], loopAfterDuration: true },
});

export const simplePatternSchema = z
  .object({
    type: z.literal('simple'),
    name: z.string().min(1, 'validation.pattern.name.required'),
    duration: z
      .number()
      .int('validation.pattern.duration.integer')
      .positive('validation.pattern.duration.min'),
    changeAt: z
      .array(patternChangeSchema)
      .min(1, 'validation.pattern.simple.changeEventRequired')
      .superRefine((changes, ctx) => {
        const timestamps = new Set<number>();
        for (const change of changes) {
          if (timestamps.has(change.ms)) {
            ctx.addIssue({
              code: 'custom',
              message: 'validation.pattern.simple.timestamp.unique',
            });
            break;
          }
          timestamps.add(change.ms);
        }
      }),
  })
  .describe('A pattern defining how an LED output changes over time.')
  .superRefine((pattern, ctx) => {
    // Check for zero-duration steps
    // A step's duration is the difference between its start time and the next step's start time (or total duration)
    const sortedChanges = [...pattern.changeAt].sort((a, b) => a.ms - b.ms);

    for (let i = 0; i < sortedChanges.length; i++) {
      const current = sortedChanges[i];
      const nextMs = i === sortedChanges.length - 1 ? pattern.duration : sortedChanges[i + 1].ms;
      const duration = nextMs - current.ms;

      if (duration <= 0) {
        ctx.addIssue({
          code: 'custom',
          message: createLocalizedError('validation.pattern.simple.stepDurationZero', {
            step: i + 1,
          }),
          path: ['changeAt', i],
        });
      }
    }
  });

export type SimplePattern = z.infer<typeof simplePatternSchema>;

export const modePatternSchema = z.discriminatedUnion('type', [
  simplePatternSchema,
  equationPatternSchema,
]);

export type ModePattern = z.infer<typeof modePatternSchema>;

export const modeComponentSchema = z
  .object({
    pattern: modePatternSchema,
  })
  .describe('Concrete pattern assignment for an LED component.');

export type ModeComponent = z.infer<typeof modeComponentSchema>;

export const modeAccelTriggerSchema = z
  .object({
    threshold: z.number().nonnegative('validation.accel.thresholdNegative'),
    front: modeComponentSchema.optional(),
    case: modeComponentSchema.optional(),
  })
  .refine(trigger => Boolean(trigger.front ?? trigger.case), 'validation.accel.componentRequired')
  .describe('Defines a pattern swap when the accelerometer exceeds a threshold.');

export type ModeAccelTrigger = z.infer<typeof modeAccelTriggerSchema>;

export const modeAccelSchema = z
  .object({
    triggers: z.array(modeAccelTriggerSchema).min(1, 'validation.accel.triggerRequired'),
  })
  .describe('Accelerometer-driven pattern overrides.');

export type ModeAccel = z.infer<typeof modeAccelSchema>;

export const modeSchema = z
  .object({
    name: z.string().min(1, 'validation.mode.nameEmpty'),
    front: modeComponentSchema.optional(),
    case: modeComponentSchema.optional(),
    accel: modeAccelSchema.optional(),
  })
  .refine(data => data.front ?? data.case, {
    message: 'validation.mode.patternRequired',
    path: ['front', 'case'],
  })
  .describe('Complete mode description including optional accelerometer triggers.');

export type Mode = z.infer<typeof modeSchema>;

export const modeDocumentSchema = z
  .object({
    mode: modeSchema,
  })
  .describe('Top-level envelope for persisted mode definitions.');

export type ModeDocument = z.infer<typeof modeDocumentSchema>;

export const parseModeDocument = (input: unknown): ModeDocument => modeDocumentSchema.parse(input);

export const isBinaryPattern = (
  pattern: ModePattern,
): pattern is SimplePattern & { changeAt: BinaryPatternChange[] } =>
  pattern.type === 'simple' &&
  pattern.changeAt.every(change => change.output === 'high' || change.output === 'low');

export const isColorPattern = (
  pattern: ModePattern,
): pattern is SimplePattern & { changeAt: ColorPatternChange[] } =>
  pattern.type === 'simple' &&
  pattern.changeAt.every(change => change.output !== 'high' && change.output !== 'low');
