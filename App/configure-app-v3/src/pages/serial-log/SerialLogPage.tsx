import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import { useSerialStore } from '@/app/providers/serial-store';
import {
  SerialLogPanel,
  type SerialLogPanelProps,
  type SerialLogState,
} from '@/components/serial-log/SerialLogPanel';

export const SerialLogPage = () => {
  const { t } = useTranslation();

  const logs = useSerialStore(s => s.logs);
  const autoscroll = useSerialStore(s => s.autoscroll);
  const status = useSerialStore(s => s.status);
  const isSupported = useSerialStore(s => s.isSupported);

  const connect = useSerialStore(s => s.connect);
  const disconnect = useSerialStore(s => s.disconnect);
  const send = useSerialStore(s => s.send);
  const clearLogs = useSerialStore(s => s.clearLogs);
  const setAutoscroll = useSerialStore(s => s.setAutoscroll);

  const [pendingPayload, setPendingPayload] = useState('');

  const panelState: SerialLogState = {
    entries: logs,
    autoscroll,
    pendingPayload,
  };

  const handleChange: SerialLogPanelProps['onChange'] = (_newState, action) => {
    switch (action.type) {
      case 'update-payload':
        setPendingPayload(action.value);
        break;
      case 'toggle-autoscroll':
        setAutoscroll(action.autoscroll);
        break;
      case 'clear':
        clearLogs();
        break;
      case 'submit':
        void send(action.entry.payload);
        setPendingPayload('');
        break;
    }
  };

  return (
    <section className="space-y-6">
      <header className="flex flex-wrap items-start justify-between gap-4">
        <div className="space-y-2">
          <h2 className="text-3xl font-semibold">{t('serialLog.title')}</h2>
          <p className="theme-muted">{t('serialLog.subtitle')}</p>
        </div>

        {isSupported ? (
          <button
            onClick={() => {
              void (status === 'connected' ? disconnect() : connect());
            }}
            className={`rounded-full px-4 py-2 text-sm font-medium transition-colors ${
              status === 'connected'
                ? 'bg-red-100 text-red-700 hover:bg-red-200 dark:bg-red-900/30 dark:text-red-300'
                : 'bg-[rgb(var(--accent)/1)] text-[rgb(var(--surface-contrast)/1)] hover:scale-[1.01]'
            }`}
          >
            {status === 'connected'
              ? t('serialLog.actions.disconnect')
              : t('serialLog.actions.connect')}
          </button>
        ) : (
          <div className="rounded-lg bg-red-50 px-3 py-2 text-sm text-red-600 dark:bg-red-900/20 dark:text-red-400">
            {t('serialLog.notSupported')}
          </div>
        )}
      </header>

      <div className="flex items-center gap-2 text-sm theme-muted">
        <div
          className={`h-2.5 w-2.5 rounded-full transition-colors ${
            status === 'connected'
              ? 'bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.4)]'
              : status === 'connecting'
                ? 'bg-yellow-500 animate-pulse'
                : 'bg-gray-300 dark:bg-gray-600'
          }`}
        />
        <span className="capitalize">{status}</span>
      </div>

      <SerialLogPanel onChange={handleChange} value={panelState} />
    </section>
  );
};
