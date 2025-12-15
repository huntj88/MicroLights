import { useTranslation } from 'react-i18next';

import { useSerialStore } from '@/app/providers/serial-store';

import { StyledButton } from './StyledButton';

export const SerialConnectButton = () => {
  const { t } = useTranslation();
  const status = useSerialStore(s => s.status);
  const isSupported = useSerialStore(s => s.isSupported);
  const connect = useSerialStore(s => s.connect);
  const disconnect = useSerialStore(s => s.disconnect);

  if (!isSupported) {
    return (
      <div className="rounded-lg bg-red-50 px-3 py-2 text-sm text-red-600 dark:bg-red-900/20 dark:text-red-400">
        {t('serialLog.notSupported')}
      </div>
    );
  }

  return (
    <StyledButton
      onClick={() => {
        void (status === 'connected' ? disconnect() : connect());
      }}
      variant={status === 'connected' ? 'danger' : 'primary'}
    >
      {status === 'connected' ? t('serialLog.actions.disconnect') : t('serialLog.actions.connect')}
    </StyledButton>
  );
};
