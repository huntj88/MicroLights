export type ChromaChannelKey = 'red' | 'green' | 'blue';

export type ChromaSynthSection = {
  id: string;
  label?: string;
  durationMs: number;
  equation: string;
};

export type ChromaSynthChannelState = {
  sections: ChromaSynthSection[];
};

export type ChromaSynthState = Record<ChromaChannelKey, ChromaSynthChannelState>;

export type ChromaSynthFrame = {
  timeMs: number;
  red: number;
  green: number;
  blue: number;
};

export type ChromaSynthExportFormat = 'mp4' | 'gif' | 'png-sequence';
