import { z } from 'zod';

const binaryOutputs = ['high', 'low'] as const;

export const binaryOutputSchema = z.enum(binaryOutputs).describe(
  'Binary output levels used by digital LEDs.',
);

export type BinaryOutput = z.infer<typeof binaryOutputSchema>;

const HEX_COLOR_REGEX = /^#(?:[0-9a-fA-F]{6})$/;

export const hexColorSchema = z
  .string()
  .regex(HEX_COLOR_REGEX, {
    message: 'Colors must be provided as 6-digit hexadecimal values such as #ffaa00.',
  })
  .brand('HexColor');

export type HexColor = z.infer<typeof hexColorSchema>;

export const patternChangeBinarySchema = z.object({
  ms: z
    .number()
    .int('Timestamps must be integers representing milliseconds.')
    .min(0, 'Timestamps cannot be negative.'),
  output: binaryOutputSchema,
});

export type BinaryPatternChange = z.infer<typeof patternChangeBinarySchema>;

export const patternChangeColorSchema = z.object({
  ms: z
    .number()
    .int('Timestamps must be integers representing milliseconds.')
    .min(0, 'Timestamps cannot be negative.'),
  output: hexColorSchema,
});

export type ColorPatternChange = z.infer<typeof patternChangeColorSchema>;

export const patternChangeSchema = z.union([
  patternChangeBinarySchema,
  patternChangeColorSchema,
]);

export type PatternChange = z.infer<typeof patternChangeSchema>;

export const equationSectionSchema = z.object({
  id: z.uuid(),
  equation: z.string().min(1, 'Equation cannot be empty'),
  duration: z.number().min(1, 'Duration must be at least 1ms'),
});

export type EquationSection = z.infer<typeof equationSectionSchema>;

export const channelConfigSchema = z.object({
  sections: z.array(equationSectionSchema),
  loopAfterDuration: z.boolean().default(true),
});

export type ChannelConfig = z.infer<typeof channelConfigSchema>;

export const equationPatternSchema = z.object({
  type: z.literal('equation'),
  id: z.uuid().optional(),
  name: z.string().min(1, 'Name is required'),
  duration: z.number().nonnegative(),
  red: channelConfigSchema,
  green: channelConfigSchema,
  blue: channelConfigSchema,
});

export type EquationPattern = z.infer<typeof equationPatternSchema>;

export const createDefaultEquationPattern = (): EquationPattern => ({
  type: 'equation',
  id: crypto.randomUUID(),
  name: 'New Equation Pattern',
  duration: 1000,
  red: { sections: [], loopAfterDuration: true },
  green: { sections: [], loopAfterDuration: true },
  blue: { sections: [], loopAfterDuration: true },
});

export const simplePatternSchema = z
  .object({
    type: z.literal('simple').default('simple'),
    name: z.string().min(1, 'Pattern name cannot be empty.'),
    duration: z
      .number()
      .int('Pattern duration must be a whole number in milliseconds.')
      .positive('Pattern duration must be greater than zero.'),
    changeAt: z
      .array(patternChangeSchema)
      .min(1, 'At least one change event is required for a pattern.')
      .superRefine((changes, ctx) => {
        const timestamps = new Set<number>();
        for (const change of changes) {
          if (timestamps.has(change.ms)) {
            ctx.addIssue({
              code: 'custom',
              message: 'Each change event must use a unique millisecond timestamp.',
            });
            break;
          }
          timestamps.add(change.ms);
        }
      }),
  })
  .describe('A pattern defining how an LED output changes over time.');

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
    threshold: z
      .number()
      .nonnegative('Accelerometer thresholds cannot be negative.'),
    front: modeComponentSchema.optional(),
    case: modeComponentSchema.optional(),
  })
  .refine(
    (trigger) => Boolean(trigger.front ?? trigger.case),
    'Accelerometer triggers must configure at least one LED component.',
  )
  .describe('Defines a pattern swap when the accelerometer exceeds a threshold.');

export type ModeAccelTrigger = z.infer<typeof modeAccelTriggerSchema>;

export const modeAccelSchema = z
  .object({
    triggers: z
      .array(modeAccelTriggerSchema)
      .min(1, 'At least one accelerometer trigger is required when accel is present.'),
  })
  .describe('Accelerometer-driven pattern overrides.');

export type ModeAccel = z.infer<typeof modeAccelSchema>;

export const modeSchema = z
  .object({
    name: z.string().min(1, 'Mode name cannot be empty.'),
    front: modeComponentSchema,
    case: modeComponentSchema,
    accel: modeAccelSchema.optional(),
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
  pattern.changeAt.every((change) => change.output === 'high' || change.output === 'low');

export const isColorPattern = (
  pattern: ModePattern,
): pattern is SimplePattern & { changeAt: ColorPatternChange[] } =>
  pattern.type === 'simple' &&
  pattern.changeAt.every((change) => change.output !== 'high' && change.output !== 'low');
