import { useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { useSerialStore } from '@/app/providers/serial-store';
import { SerialConnectButton } from '@/components/common/SerialConnectButton';
import { StyledButton } from '@/components/common/StyledButton';
import {
  SerialLogPanel,
  type SerialLogPanelProps,
  type SerialLogState,
} from '@/components/serial-log/SerialLogPanel';
import { SettingsModal } from '@/components/serial-log/SettingsModal';

export const SerialLogPage = () => {
  const { t } = useTranslation();

  const isMobileOS = useMemo(
    () => /android|iphone|ipad|ipod/i.test(navigator.userAgent),
    [],
  );

  const logs = useSerialStore(s => s.logs);
  const status = useSerialStore(s => s.status);

  const send = useSerialStore(s => s.send);
  const clearLogs = useSerialStore(s => s.clearLogs);

  const [pendingPayload, setPendingPayload] = useState('');
  const [isSettingsModalOpen, setIsSettingsModalOpen] = useState(false);

  const panelState: SerialLogState = {
    entries: logs,
    pendingPayload,
  };

  const handleChange: SerialLogPanelProps['onChange'] = (_newState, action) => {
    switch (action.type) {
      case 'update-payload':
        setPendingPayload(action.value);
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
    <section className="space-y-4 sm:space-y-6">
      <header className="flex flex-col gap-2 sm:flex-row sm:flex-wrap sm:items-start sm:justify-between sm:gap-4">
        <div className="space-y-2">
          <h2 className="text-xl font-semibold sm:text-3xl">{t('serialLog.title')}</h2>
          <p className="text-sm theme-muted">{t('serialLog.subtitle')}</p>
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
                : status === 'disconnecting'
                  ? 'bg-yellow-400 animate-pulse'
                  : status === 'error'
                    ? 'bg-red-500'
                    : 'bg-[rgb(var(--surface-muted)/0.4)]'
          }`}
        />
        <span className="capitalize">{status}</span>
      </div>

      <div className="flex flex-col gap-2 sm:flex-row sm:flex-wrap sm:gap-2">
        <StyledButton
          onClick={() => {
            void (async () => {
              for (let i = 0; i < 6; i++) {
                await send({ command: 'readMode', index: i });
              }
            })();
          }}
          disabled={status !== 'connected'}
        >
          Read Modes
        </StyledButton>
        <StyledButton
          onClick={() => {
            setIsSettingsModalOpen(true);
          }}
          disabled={status !== 'connected'}
        >
          Settings
        </StyledButton>
        <StyledButton
          onClick={() => void send({ command: 'dfu' })}
          disabled={isMobileOS || status !== 'connected'}
          title={isMobileOS ? 'DFU is only available on desktop' : undefined}
        >
          DFU
        </StyledButton>
        {isMobileOS && (
          <p className="text-xs theme-muted">DFU is only available on desktop</p>
        )}
      </div>

      <SerialLogPanel onChange={handleChange} value={panelState} />

      <SettingsModal
        isOpen={isSettingsModalOpen}
        onClose={() => {
          setIsSettingsModalOpen(false);
        }}
      />
    </section>
  );
};
