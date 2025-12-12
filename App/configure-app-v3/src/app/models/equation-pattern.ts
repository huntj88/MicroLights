import { z } from 'zod';

export const equationSectionSchema = z.object({
  id: z.string().uuid(),
  equation: z.string().min(1, 'Equation cannot be empty'),
  duration: z.number().min(1, 'Duration must be at least 1ms'),
});

export type EquationSection = z.infer<typeof equationSectionSchema>;

export const channelConfigSchema = z.object({
  sections: z.array(equationSectionSchema),
});

export type ChannelConfig = z.infer<typeof channelConfigSchema>;

export const equationPatternSchema = z.object({
  id: z.string().uuid(),
  name: z.string().min(1, 'Name is required'),
  red: channelConfigSchema,
  green: channelConfigSchema,
  blue: channelConfigSchema,
});

export type EquationPattern = z.infer<typeof equationPatternSchema>;

export const createDefaultEquationPattern = (): EquationPattern => ({
  id: crypto.randomUUID(),
  name: 'New Equation Pattern',
  red: { sections: [] },
  green: { sections: [] },
  blue: { sections: [] },
});
