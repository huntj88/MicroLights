import { useState, useMemo } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { type Mode, type ModePattern } from '@/app/models/mode';
import { useModeStore } from '@/app/providers/mode-store';
import { usePatternStore } from '@/app/providers/pattern-store';

import { StyledButton } from '../common/StyledButton';

interface ImportModeModalProps {
  isOpen: boolean;
  onClose: () => void;
  mode: Mode;
}

export const ImportModeModal = ({ isOpen, onClose, mode }: ImportModeModalProps) => {
  const { t } = useTranslation();
  const saveMode = useModeStore(s => s.saveMode);
  const getMode = useModeStore(s => s.getMode);
  const savePattern = usePatternStore(s => s.savePattern);
  const getPattern = usePatternStore(s => s.getPattern);

  const [modeName, setModeName] = useState(mode.name);
  const [importMode, setImportMode] = useState(true);
  const [selectedPatterns, setSelectedPatterns] = useState<Set<string>>(new Set());

  const patterns = useMemo(() => {
    const uniquePatterns = new Map<string, ModePattern>();
    
    const addPattern = (p?: ModePattern) => {
      if (p) uniquePatterns.set(p.name, p);
    };

    addPattern(mode.front?.pattern);
    addPattern(mode.case?.pattern);
    
    mode.accel?.triggers.forEach(trigger => {
      addPattern(trigger.front?.pattern);
      addPattern(trigger.case?.pattern);
    });

    return Array.from(uniquePatterns.values());
  }, [mode]);

  if (!isOpen) return null;

  const existingMode = getMode(modeName);
  const isModeOverwrite = !!existingMode && importMode;

  const overwritingPatterns = patterns.filter(
    p => selectedPatterns.has(p.name) && !!getPattern(p.name)
  );
  const isPatternOverwrite = overwritingPatterns.length > 0;
  const isOverwrite = isModeOverwrite || isPatternOverwrite;

  const togglePattern = (name: string) => {
    const next = new Set(selectedPatterns);
    if (next.has(name)) {
      next.delete(name);
    } else {
      next.add(name);
    }
    setSelectedPatterns(next);
  };

  const handleImport = () => {
    try {
      let importedCount = 0;

      if (importMode) {
        saveMode({ ...mode, name: modeName });
        importedCount++;
      }

      patterns.forEach(p => {
        if (selectedPatterns.has(p.name)) {
          savePattern(p);
          importedCount++;
        }
      });

      if (importedCount > 0) {
        toast.success(t('serialLog.importMode.success'));
        onClose();
      } else {
        toast.error(t('serialLog.importMode.nothingSelected'));
      }
    } catch (error) {
      console.error('Failed to import mode', error);
      toast.error(t('serialLog.importMode.error'));
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 p-4 backdrop-blur-sm">
      <div className="flex w-full max-w-md flex-col rounded-xl bg-white p-6 shadow-xl dark:bg-gray-800 max-h-[90vh] overflow-y-auto">
        <header className="mb-4 flex items-center justify-between">
          <h3 className="text-xl font-semibold">{t('serialLog.importMode.title')}</h3>
          <button
            onClick={onClose}
            className="rounded-lg p-2 hover:bg-gray-100 dark:hover:bg-gray-700"
          >
            ✕
          </button>
        </header>

        <div className="space-y-4">
          <p className="text-sm text-gray-600 dark:text-gray-300">
            {t('serialLog.importMode.description')}
          </p>

          <div className="space-y-2 rounded-lg border border-gray-200 p-3 dark:border-gray-700">
            <div className="flex items-center gap-2">
              <input
                type="checkbox"
                id="importMode"
                checked={importMode}
                onChange={e => setImportMode(e.target.checked)}
                className="h-4 w-4 rounded border-gray-300 text-blue-600 focus:ring-blue-500"
              />
              <label htmlFor="importMode" className="font-medium">
                {t('serialLog.importMode.importFullMode')}
              </label>
            </div>
            <p className="ml-6 text-xs text-gray-500 dark:text-gray-400">
              {t('serialLog.importMode.modeHelp')}
            </p>

            {importMode && (
              <div className="mt-2 pl-6">
                <label htmlFor="modeName" className="block text-xs font-medium text-gray-500">
                  {t('common.labels.name', 'Name')}
                </label>
                <input
                  id="modeName"
                  type="text"
                  value={modeName}
                  onChange={e => setModeName(e.target.value)}
                  className="mt-1 w-full rounded-lg border border-gray-300 bg-transparent px-3 py-2 text-sm focus:border-blue-500 focus:outline-none dark:border-gray-600"
                />
                {isModeOverwrite && (
                  <div className="mt-2 text-xs text-yellow-600 dark:text-yellow-400">
                    {t('serialLog.importMode.overwriteWarning')}
                  </div>
                )}
              </div>
            )}
          </div>

          {patterns.length > 0 && (
            <div className="space-y-2">
              <div className="flex items-center justify-between">
                <h4 className="text-sm font-medium">{t('serialLog.importMode.patterns')}</h4>
              </div>
              <p className="text-xs text-gray-500 dark:text-gray-400">
                {t('serialLog.importMode.patternsHelp')}
              </p>
              <div className="space-y-2 rounded-lg border border-gray-200 p-3 dark:border-gray-700">
                {patterns.map(pattern => {
                  const exists = !!getPattern(pattern.name);
                  return (
                    <div key={pattern.name} className="flex flex-col gap-1">
                      <div className="flex items-center gap-2">
                        <input
                          type="checkbox"
                          id={`pattern-${pattern.name}`}
                          checked={selectedPatterns.has(pattern.name)}
                          onChange={() => togglePattern(pattern.name)}
                          className="h-4 w-4 rounded border-gray-300 text-blue-600 focus:ring-blue-500"
                        />
                        <label htmlFor={`pattern-${pattern.name}`} className="text-sm">
                          {pattern.name} <span className="text-xs text-gray-500">({pattern.type})</span>
                        </label>
                      </div>
                      {exists && selectedPatterns.has(pattern.name) && (
                        <div className="ml-6 text-xs text-yellow-600 dark:text-yellow-400">
                          {t('serialLog.importMode.overwritePatternWarning')}
                        </div>
                      )}
                    </div>
                  );
                })}
              </div>
            </div>
          )}

          <div className="rounded-lg border border-gray-200 bg-gray-50 p-3 text-xs font-mono dark:border-gray-700 dark:bg-gray-900">
            <div className="mb-1 text-xs font-semibold text-gray-500">JSON Preview</div>
            <pre className="max-h-32 overflow-y-auto whitespace-pre-wrap break-all">
              {JSON.stringify(mode, null, 2)}
            </pre>
          </div>
        </div>

        <footer className="mt-6 flex justify-end gap-3 border-t border-gray-100 pt-4 dark:border-gray-700">
          <StyledButton onClick={onClose} variant="secondary">
            {t('common.actions.cancel')}
          </StyledButton>
          <StyledButton onClick={handleImport} variant="primary" disabled={!importMode && selectedPatterns.size === 0}>
            {isOverwrite ? t('common.actions.overwrite') : t('common.actions.import')}
          </StyledButton>
        </footer>
      </div>
    </div>
  );
};
