import { useEffect, useState } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { serialManager } from '@/app/providers/serial-manager';
import { StyledButton } from '@/components/common/StyledButton';

interface SettingsModalProps {
  isOpen: boolean;
  onClose: () => void;
}

interface SettingsData {
  modeCount: number;
  minutesUntilAutoOff: number;
  minutesUntilLockAfterAutoOff: number;
}

export const SettingsModal = ({ isOpen, onClose }: SettingsModalProps) => {
  const { t } = useTranslation();
  const [isLoading, setIsLoading] = useState(false);
  const [data, setData] = useState<SettingsData>({
    modeCount: 5,
    minutesUntilAutoOff: 30,
    minutesUntilLockAfterAutoOff: 60,
  });

  // Reset state when opening
  useEffect(() => {
    if (isOpen) {
      setIsLoading(true);
      // Send read command
      void serialManager.send({ command: 'readSettings' });

      // TODO: if no response in X seconds, show error instead of showing defaults
      // Timeout fallback
      const timer = setTimeout(() => {
        setIsLoading(false);
      }, 3000);

      // Setup listener for response
      const cleanup = serialManager.on('data', (_line, json) => {
        if (
          json &&
          typeof json === 'object' &&
          'modeCount' in json &&
          'minutesUntilAutoOff' in json &&
          'minutesUntilLockAfterAutoOff' in json
        ) {
          setData(json as SettingsData);
          setIsLoading(false);
          toast.success('Settings loaded');
          clearTimeout(timer);
        }
      });

      return () => {
        cleanup();
        clearTimeout(timer);
      };
    }
  }, [isOpen]);

  const handleSave = async () => {
    try {
      await serialManager.send({
        command: 'writeSettings',
        ...data,
      });
      toast.success('Settings saved');
      onClose();
    } catch (error) {
      console.error(error);
      toast.error('Failed to save settings');
    }
  };

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50">
      <div className="w-full max-w-md rounded-lg bg-white p-6 shadow-xl dark:bg-gray-800">
        <h2 className="mb-4 text-xl font-bold text-gray-900 dark:text-white">Configure Settings</h2>

        {isLoading ? (
          <div className="flex justify-center py-8">
            <div className="h-8 w-8 animate-spin rounded-full border-4 border-gray-300 border-t-blue-600" />
          </div>
        ) : (
          <div className="space-y-4">
            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300">
                Mode Count
              </label>
              <input
                type="number"
                className="mt-1 block w-full rounded-md border border-gray-300 bg-white px-3 py-2 text-gray-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700 dark:text-white"
                value={data.modeCount}
                onChange={e => {
                  setData({ ...data, modeCount: parseInt(e.target.value) || 0 });
                }}
              />
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300">
                Minutes Until Auto Off
              </label>
              <input
                type="number"
                className="mt-1 block w-full rounded-md border border-gray-300 bg-white px-3 py-2 text-gray-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700 dark:text-white"
                value={data.minutesUntilAutoOff}
                onChange={e => {
                  setData({
                    ...data,
                    minutesUntilAutoOff: parseInt(e.target.value) || 0,
                  });
                }}
              />
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300">
                Minutes Until Lock After Auto Off
              </label>
              <input
                type="number"
                className="mt-1 block w-full rounded-md border border-gray-300 bg-white px-3 py-2 text-gray-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700 dark:text-white"
                value={data.minutesUntilLockAfterAutoOff}
                onChange={e => {
                  setData({
                    ...data,
                    minutesUntilLockAfterAutoOff: parseInt(e.target.value) || 0,
                  });
                }}
              />
            </div>
          </div>
        )}

        <div className="mt-6 flex justify-end space-x-3">
          <StyledButton onClick={onClose} variant="secondary">
            {t('common.actions.cancel')}
          </StyledButton>
          <StyledButton onClick={() => void handleSave()} variant="primary" disabled={isLoading}>
            {t('common.actions.save')}
          </StyledButton>
        </div>
      </div>
    </div>
  );
};
