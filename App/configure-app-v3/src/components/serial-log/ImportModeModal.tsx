import { useState, useMemo, useEffect } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { type Mode, type ModePattern, type ModeComponent } from '@/app/models/mode';
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
  const [patternNames, setPatternNames] = useState<Record<string, string>>({});

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

  useEffect(() => {
    const names: Record<string, string> = {};
    patterns.forEach(p => {
      names[p.name] = p.name;
    });
    setPatternNames(names);
  }, [patterns]);

  if (!isOpen) return null;

  const existingMode = getMode(modeName);
  const isModeOverwrite = !!existingMode;

  const overwritingPatterns = patterns.filter(p => {
    const currentName = patternNames[p.name] ?? p.name;
    return !!getPattern(currentName);
  });
  const isPatternOverwrite = overwritingPatterns.length > 0;
  const isOverwrite = isModeOverwrite || isPatternOverwrite;

  const hasEmptyNames =
    !modeName.trim() ||
    patterns.some(p => {
      const name = patternNames[p.name] ?? p.name;
      return !name.trim();
    });

  const handleImport = () => {
    if (hasEmptyNames) return;
    try {
      // Create a map of oldName -> newPattern (with new name)
      const newPatternsMap = new Map<string, ModePattern>();
      patterns.forEach(p => {
        const newName = patternNames[p.name] ?? p.name;
        newPatternsMap.set(p.name, { ...p, name: newName });
      });

      const updateComponent = (comp?: ModeComponent) => {
        if (!comp?.pattern) return comp;
        const updatedPattern = newPatternsMap.get(comp.pattern.name);
        if (updatedPattern) {
          return { ...comp, pattern: updatedPattern };
        }
        return comp;
      };

      const updatedMode: Mode = {
        ...mode,
        name: modeName,
        front: updateComponent(mode.front),
        case: updateComponent(mode.case),
        accel: mode.accel
          ? {
              ...mode.accel,
              triggers: mode.accel.triggers.map(t => ({
                ...t,
                front: updateComponent(t.front),
                case: updateComponent(t.case),
              })),
            }
          : undefined,
      };

      saveMode(updatedMode);
      newPatternsMap.forEach(p => savePattern(p));

      toast.success(t('serialLog.importMode.success'));
      onClose();
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
              <span className="font-medium">{t('serialLog.importMode.importFullMode')}</span>
            </div>
            <p className="text-xs text-gray-500 dark:text-gray-400">
              {t('serialLog.importMode.modeHelp')}
            </p>

            <div className="mt-2">
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
                  const currentName = patternNames[pattern.name] ?? pattern.name;
                  const exists = !!getPattern(currentName);
                  return (
                    <div key={pattern.name} className="flex flex-col gap-1">
                      <div className="flex items-center gap-2">
                        <div className="h-2 w-2 rounded-full bg-blue-500" />
                        <input
                          type="text"
                          value={currentName}
                          onChange={e =>
                            setPatternNames(prev => ({ ...prev, [pattern.name]: e.target.value }))
                          }
                          className="w-full rounded border border-gray-300 bg-transparent px-2 py-1 text-sm focus:border-blue-500 focus:outline-none dark:border-gray-600"
                        />
                        <span className="whitespace-nowrap text-xs text-gray-500">
                          ({pattern.type})
                        </span>
                      </div>
                      {exists && (
                        <div className="ml-4 text-xs text-yellow-600 dark:text-yellow-400">
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
          <StyledButton onClick={handleImport} variant="primary" disabled={hasEmptyNames}>
            {isOverwrite ? t('common.actions.overwrite') : t('common.actions.import')}
          </StyledButton>
        </footer>
      </div>
    </div>
  );
};
