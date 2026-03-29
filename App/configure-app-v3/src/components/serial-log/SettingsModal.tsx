import { useEffect, useState } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { serialManager } from '@/app/providers/serial-manager';
import { Modal } from '@/components/common/Modal';
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

      // TODO: if no response in X seconds, show retry option
      // Timeout fallback
      const timer = setTimeout(() => {
        setIsLoading(false);
      }, 3000);

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
      const cleanup = serialManager.on('data', (_line, json) => {
        if (isValidResponse(json)) {
          handleResponse(json);
          return;
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

  return (
    <Modal
      isOpen={isOpen}
      onClose={onClose}
      title={t('serialLog.configureSettings.title')}
      maxWidth="md"
    >
      {isLoading ? (
        <div className="flex justify-center py-8">
          <div className="h-8 w-8 animate-spin rounded-full border-4 border-[rgb(var(--surface-muted)/0.3)] border-t-[rgb(var(--accent)/1)]" />
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
                    className="block text-sm font-medium theme-muted"
                  >
                    {humanizeCamelCase(key)}
                  </label>
                  {isBoolean ? (
                    <div className="mt-1 flex items-center">
                      <input
                        id={`setting-${key}`}
                        type="checkbox"
                        className="h-5 w-5 rounded border theme-border bg-[rgb(var(--surface-raised)/1)] accent-[rgb(var(--accent)/1)]"
                        checked={!!value}
                        onChange={e => {
                          setSettings(prev => ({
                            ...prev,
                            [key]: e.target.checked,
                          }));
                        }}
                      />
                      <span className="ml-2 text-sm theme-muted">
                        {value ? 'Enabled' : 'Disabled'}
                      </span>
                    </div>
                  ) : (
                    <input
                      id={`setting-${key}`}
                      type="number"
                      className="mt-1 block w-full rounded-md border theme-border bg-[rgb(var(--surface-raised)/0.5)] px-3 py-2 text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none"
                      value={value as number}
                      onChange={e => {
                        setSettings(prev => ({
                          ...prev,
                          [key]: parseInt(e.target.value) || 0,
                        }));
                      }}
                    />
                  )}
                  <p className="mt-1 text-xs theme-muted">Default: {String(defaults[key])}</p>
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
    </Modal>
  );
};
