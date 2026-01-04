import { useEffect, useState } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { serialManager } from '@/app/providers/serial-manager';
import { StyledButton } from '@/components/common/StyledButton';

interface SettingsModalProps {
  isOpen: boolean;
  onClose: () => void;
}

interface Settings {
  modeCount: number;
  minutesUntilAutoOff: number;
  minutesUntilLockAfterAutoOff: number;
  equationEvalIntervalMs: number;
}

interface SettingsResponse {
  settings: Partial<Settings> & { command: string };
  defaults: Settings;
}

export const SettingsModal = ({ isOpen, onClose }: SettingsModalProps) => {
  const { t } = useTranslation();
  const [isLoading, setIsLoading] = useState(false);
  const [settings, setSettings] = useState<Settings>({
    modeCount: 5,
    minutesUntilAutoOff: 30,
    minutesUntilLockAfterAutoOff: 60,
    equationEvalIntervalMs: 20,
  });
  const [defaults, setDefaults] = useState<SettingsResponse['defaults'] | null>(null);

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
          'settings' in json &&
          'defaults' in json
        ) {
          const response = json as SettingsResponse;
          setSettings({
            modeCount: response.settings.modeCount ?? response.defaults.modeCount,
            minutesUntilAutoOff:
              response.settings.minutesUntilAutoOff ?? response.defaults.minutesUntilAutoOff,
            minutesUntilLockAfterAutoOff:
              response.settings.minutesUntilLockAfterAutoOff ??
              response.defaults.minutesUntilLockAfterAutoOff,
            equationEvalIntervalMs:
              response.settings.equationEvalIntervalMs ??
              response.defaults.equationEvalIntervalMs,
          });
          setDefaults(response.defaults);
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
      const payload: Record<string, unknown> = {
        command: 'writeSettings',
      };

      if (defaults) {
        (Object.keys(settings) as (keyof Settings)[]).forEach(key => {
          if (settings[key] !== defaults[key]) {
            payload[key] = settings[key];
          }
        });
      } else {
        Object.assign(payload, settings);
      }

      await serialManager.send(payload);
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
                value={settings.modeCount}
                onChange={e => {
                  setSettings({ ...settings, modeCount: parseInt(e.target.value) || 0 });
                }}
              />
              {defaults && (
                <p className="mt-1 text-xs text-gray-500 dark:text-gray-400">
                  Default: {defaults.modeCount}
                </p>
              )}
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300">
                Minutes Until Auto Off
              </label>
              <input
                type="number"
                className="mt-1 block w-full rounded-md border border-gray-300 bg-white px-3 py-2 text-gray-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700 dark:text-white"
                value={settings.minutesUntilAutoOff}
                onChange={e => {
                  setSettings({
                    ...settings,
                    minutesUntilAutoOff: parseInt(e.target.value) || 0,
                  });
                }}
              />
              {defaults && (
                <p className="mt-1 text-xs text-gray-500 dark:text-gray-400">
                  Default: {defaults.minutesUntilAutoOff}
                </p>
              )}
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300">
                Minutes Until Lock After Auto Off
              </label>
              <input
                type="number"
                className="mt-1 block w-full rounded-md border border-gray-300 bg-white px-3 py-2 text-gray-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700 dark:text-white"
                value={settings.minutesUntilLockAfterAutoOff}
                onChange={e => {
                  setSettings({
                    ...settings,
                    minutesUntilLockAfterAutoOff: parseInt(e.target.value) || 0,
                  });
                }}
              />
              {defaults && (
                <p className="mt-1 text-xs text-gray-500 dark:text-gray-400">
                  Default: {defaults.minutesUntilLockAfterAutoOff}
                </p>
              )}
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-700 dark:text-gray-300">
                Equation Eval Interval (ms)
              </label>
              <input
                type="number"
                className="mt-1 block w-full rounded-md border border-gray-300 bg-white px-3 py-2 text-gray-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700 dark:text-white"
                value={settings.equationEvalIntervalMs}
                onChange={e => {
                  setSettings({
                    ...settings,
                    equationEvalIntervalMs: parseInt(e.target.value) || 0,
                  });
                }}
              />
              {defaults && (
                <p className="mt-1 text-xs text-gray-500 dark:text-gray-400">
                  Default: {defaults.equationEvalIntervalMs}
                </p>
              )}
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
