import type { TFunction } from 'i18next';

// used for zod error localization, where messages are stored as JSON strings in some cases.
interface LocalizedError {
  key: string;
  params?: Record<string, unknown>;
}

export const createLocalizedError = (key: string, params?: Record<string, unknown>): string => {
  return JSON.stringify({ key, params });
};

const parseLocalizedError = (error: string): LocalizedError | null => {
  try {
    if (error.startsWith('{')) {
      return JSON.parse(error) as LocalizedError;
    }
  } catch {
    // ignore
  }
  return null;
};

export const getLocalizedError = (error: string, t: TFunction): string => {
  const parsed = parseLocalizedError(error);
  if (parsed) {
    return t(parsed.key, parsed.params);
  }
  return t(error);
};
