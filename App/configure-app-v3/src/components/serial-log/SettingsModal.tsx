import { useEffect, useState } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { serialManager } from '@/app/providers/serial-manager';
import { StyledButton } from '@/components/common/StyledButton';
import { humanizeCamelCase } from '@/utils/string-utils';

interface SettingsModalProps {
  isOpen: boolean;
  onClose: () => void;
}

type SettingValue = number | boolean;
type Settings = Record<string, SettingValue>;

interface SettingsResponse {
  settings: Settings & { command: string };
  defaults: Settings;
}

export const SettingsModal = ({ isOpen, onClose }: SettingsModalProps) => {
  const { t } = useTranslation();
  const [isLoading, setIsLoading] = useState(false);
  const [settings, setSettings] = useState<Settings>({});
  const [defaults, setDefaults] = useState<Settings | null>(null);

  // Reset state when opening
  useEffect(() => {
    if (isOpen) {
      setIsLoading(true);
      setSettings({});
      setDefaults(null);

      // Send read command
      void serialManager.send({ command: 'readSettings' });

      // TODO: if no response in X seconds, show error instead of showing defaults
      // Timeout fallback
      const timer = setTimeout(() => {
        setIsLoading(false);
      }, 3000);

      let buffer = '';

      const handleResponse = (response: SettingsResponse) => {
        const newSettings: Settings = {};

        // Initialize with defaults
        Object.keys(response.defaults).forEach(key => {
          newSettings[key] = response.defaults[key];
        });

        // Override with current settings
        Object.keys(response.settings).forEach(key => {
          if (key !== 'command') {
            newSettings[key] = response.settings[key];
          }
        });

        setSettings(newSettings);
        setDefaults(response.defaults);
        setIsLoading(false);
        toast.success('Settings loaded');
        clearTimeout(timer);
      };

      const isValidResponse = (json: unknown): json is SettingsResponse => {
        return !!json && typeof json === 'object' && 'settings' in json && 'defaults' in json;
      };

      // Setup listener for response
      const cleanup = serialManager.on('data', (line, json) => {
        if (isValidResponse(json)) {
          handleResponse(json);
          return;
        }

        // Try buffering
        buffer += line;
        try {
          const parsed: unknown = JSON.parse(buffer);
          if (isValidResponse(parsed)) {
            handleResponse(parsed);
          }
        } catch {
          // ignore
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
        Object.keys(settings).forEach(key => {
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
      <div className="w-full max-w-md rounded-lg bg-white p-6 shadow-xl dark:bg-gray-800 max-h-[90vh] overflow-y-auto">
        <h2 className="mb-4 text-xl font-bold text-gray-900 dark:text-white">Configure Settings</h2>

        {isLoading ? (
          <div className="flex justify-center py-8">
            <div className="h-8 w-8 animate-spin rounded-full border-4 border-gray-300 border-t-blue-600" />
          </div>
        ) : (
          <div className="space-y-4">
            {defaults &&
              Object.keys(defaults).map(key => {
                const isBoolean = typeof defaults[key] === 'boolean';
                const value = settings[key];

                return (
                  <div key={key}>
                    <label
                      htmlFor={`setting-${key}`}
                      className="block text-sm font-medium text-gray-700 dark:text-gray-300"
                    >
                      {humanizeCamelCase(key)}
                    </label>
                    {isBoolean ? (
                      <div className="mt-1 flex items-center">
                        <input
                          id={`setting-${key}`}
                          type="checkbox"
                          className="h-4 w-4 rounded border-gray-300 text-blue-600 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700"
                          checked={!!value}
                          onChange={e => {
                            setSettings(prev => ({
                              ...prev,
                              [key]: e.target.checked,
                            }));
                          }}
                        />
                        <span className="ml-2 text-sm text-gray-500 dark:text-gray-400">
                          {value ? 'Enabled' : 'Disabled'}
                        </span>
                      </div>
                    ) : (
                      <input
                        id={`setting-${key}`}
                        type="number"
                        className="mt-1 block w-full rounded-md border border-gray-300 bg-white px-3 py-2 text-gray-900 focus:border-blue-500 focus:outline-none focus:ring-1 focus:ring-blue-500 dark:border-gray-600 dark:bg-gray-700 dark:text-white"
                        value={value as number}
                        onChange={e => {
                          setSettings(prev => ({
                            ...prev,
                            [key]: parseInt(e.target.value) || 0,
                          }));
                        }}
                      />
                    )}
                    <p className="mt-1 text-xs text-gray-500 dark:text-gray-400">
                      Default: {String(defaults[key])}
                    </p>
                  </div>
                );
              })}
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
