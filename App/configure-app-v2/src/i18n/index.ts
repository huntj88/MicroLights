import { createInstance } from 'i18next';
import LanguageDetector from 'i18next-browser-languagedetector';
import { initReactI18next } from 'react-i18next';

// Default (en) translations inline for now; can be split per-namespace later
const resources = {
  en: {
    common: {
      appName: 'BulbChips',
      workInProgress: 'Work in progress.',
      create: 'Create',
      browse: 'Browse',
      program: 'Program',
      extra: 'Extra',
      mode: 'Mode',
      wave: 'Wave',
      settings: 'Settings',
      docs: 'Documentation',
      examples: 'Examples',
      unsavedDraft: '(Unsaved Draft)',
      newDraft: 'New Draft',
      delete: 'Delete',
      save: 'Save',
      addToLibrary: 'Add to Library',
      name: 'Name',
      totalTicks: 'Total Ticks',
      connect: 'Connect',
      disconnect: 'Disconnect',
      send: 'Send',
      copy: 'Copy',
      remove: 'Remove',
      none: 'None',
      cancel: 'Cancel',
      editFullscreen: 'Edit Fullscreen',
      saveAndUse: 'Save and Use',
      left: 'Left',
      right: 'Right',
      fingers: 'Fingers',
      waveform: 'Waveform',
      accelerometer: 'Accelerometer',
      caseLightColor: 'Case Light Color',
      exportJSON: 'Export (JSON)',
      addTrigger: '+ Add Trigger',
      addModeCard: '+ Add Mode Card',
      addNewMode: 'Add a new Mode',
      maxModesReached: 'Maximum number of Modes reached',
      editWaveform: 'Edit Waveform',
      newWaveform: 'New Waveform',
      thresholdXg: 'Threshold (× g)',
      enableCaseLight: 'Enable case light',
      caseLightColorAria: 'Case light color',
      enableCaseLightToChoose: 'Enable case light to choose color',
      accelerationThresholdAria: 'Acceleration threshold in multiples of g',
      enableTriggerColor: 'Enable trigger color',
      triggerColor: 'Trigger color',
      enableTriggerColorToChoose: 'Enable trigger color to choose color',
      l: 'L',
      r: 'R',
      tipWaveformEditor:
        'Tip: Click the timeline to add a marker (above center = high, below = low). Drag to move. Double-click a marker to delete. Use Ctrl+Z / Ctrl+Shift+Z to undo/redo.',
      undo: 'Undo',
      redo: 'Redo',
      jsonConfig: 'Json Config',
      close: 'Close',
      appearance: 'Appearance',
      theme: 'Theme',
      system: 'System',
      light: 'Light',
      dark: 'Dark',
      systemThemeHint:
        'When set to System, the app follows your OS setting (prefers-color-scheme).',
      tip: 'Tip:',
      tipModeSave:
        'Save the current configuration as a Mode, or load an existing Mode to edit and save changes.',
      modeCardTipPlural: 'Finger-specific options are shown below for each Mode Card.',
      modeCardTipSingle: 'Add another Mode Card to reveal finger-specific options.',
      createModeTitle: 'Create / Mode',
      createWaveTitle: 'Create / Wave',
      browseTitle: 'Browse',
      programTitle: 'Program',
      docsTitle: 'Docs',
      examplesTitle: 'Examples',
      // toasts and confirmations
      confirmDeleteMode: 'Delete this mode?',
      modeLoaded: 'Mode loaded',
      modeSaved: 'Mode saved',
      addedToLibrary: 'Added to Library',
      programSent: 'Program sent',
      invalidWaveform: 'Invalid waveform',
      cannotDeleteFirstMarker: 'The first marker at tick 0 cannot be deleted',
      draft: 'Draft',
      setN: 'Set {{n}}',
      pulse: 'Pulse',
      newMode: 'New Mode',
      newWave: 'New Wave',
      fingerLabel: {
        'L-thumb': 'L thumb',
        'L-index': 'L index',
        'L-middle': 'L middle',
        'L-ring': 'L ring',
        'L-pinky': 'L pinky',
        'R-thumb': 'R thumb',
        'R-index': 'R index',
        'R-middle': 'R middle',
        'R-ring': 'R ring',
        'R-pinky': 'R pinky',
      },
    },
  },
} as const;

// Helper to generate Set 1, Set 2, etc
function tSetN(n: number) {
  return `Set ${n}`;
}

const i18n = createInstance();

void i18n
  .use(LanguageDetector)
  .use(initReactI18next)
  .init({
    resources,
    fallbackLng: 'en',
    ns: ['common'],
    defaultNS: 'common',
    interpolation: { escapeValue: false },
    detection: {
      order: ['localStorage', 'navigator', 'htmlTag'],
      caches: ['localStorage'],
    },
  });

export default i18n;
export { tSetN };
