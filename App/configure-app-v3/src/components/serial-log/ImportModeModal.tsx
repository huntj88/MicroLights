import { useState } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { type Mode } from '@/app/models/mode';
import { useModeStore } from '@/app/providers/mode-store';

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

  const [modeName, setModeName] = useState(mode.name);

  if (!isOpen) return null;

  const existingMode = getMode(modeName);
  const isOverwrite = !!existingMode;

  const handleImport = () => {
    try {
      saveMode({ ...mode, name: modeName });
      toast.success(t('serialLog.importMode.success'));
      onClose();
    } catch (error) {
      console.error('Failed to import mode', error);
      toast.error(t('serialLog.importMode.error'));
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 p-4 backdrop-blur-sm">
      <div className="flex w-full max-w-md flex-col rounded-xl bg-white p-6 shadow-xl dark:bg-gray-800">
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

          <div className="space-y-2">
            <label htmlFor="modeName" className="block text-sm font-medium">
              {t('common.labels.name', 'Name')}
            </label>
            <input
              id="modeName"
              type="text"
              value={modeName}
              onChange={e => setModeName(e.target.value)}
              className="w-full rounded-lg border border-gray-300 bg-transparent px-3 py-2 text-sm focus:border-blue-500 focus:outline-none dark:border-gray-600"
            />
          </div>

          {isOverwrite && (
            <div className="rounded-lg bg-yellow-50 p-3 text-sm text-yellow-800 dark:bg-yellow-900/30 dark:text-yellow-200">
              {t('serialLog.importMode.overwriteWarning')}
            </div>
          )}

          <div className="rounded-lg border border-gray-200 bg-gray-50 p-3 text-xs font-mono dark:border-gray-700 dark:bg-gray-900">
            <pre className="overflow-x-auto whitespace-pre-wrap break-all">
              {JSON.stringify(mode, null, 2)}
            </pre>
          </div>
        </div>

        <footer className="mt-6 flex justify-end gap-3 border-t border-gray-100 pt-4 dark:border-gray-700">
          <StyledButton onClick={onClose} variant="secondary">
            {t('common.actions.cancel')}
          </StyledButton>
          <StyledButton onClick={handleImport} variant="primary">
            {isOverwrite ? t('common.actions.overwrite') : t('common.actions.import')}
          </StyledButton>
        </footer>
      </div>
    </div>
  );
};
