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

export const modePatternSchema = z
  .object({
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
): pattern is ModePattern & { changeAt: BinaryPatternChange[] } =>
  pattern.changeAt.every((change) => change.output === 'high' || change.output === 'low');

export const isColorPattern = (
  pattern: ModePattern,
): pattern is ModePattern & { changeAt: ColorPatternChange[] } =>
  pattern.changeAt.every((change) => change.output !== 'high' && change.output !== 'low');
