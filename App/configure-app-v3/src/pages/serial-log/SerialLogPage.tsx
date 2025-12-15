import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import { useSerialStore } from '@/app/providers/serial-store';
import { SerialConnectButton } from '@/components/common/SerialConnectButton';
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

        <SerialConnectButton />
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
