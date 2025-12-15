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

  // Determine button state
  const isTransitioning = status === 'connecting' || status === 'disconnecting';
  let buttonText: string;
  if (status === 'connecting') {
    buttonText = t('serialLog.actions.connecting');
  } else if (status === 'disconnecting') {
    buttonText = t('serialLog.actions.disconnecting');
  } else if (status === 'connected') {
    buttonText = t('serialLog.actions.disconnect');
  } else {
    buttonText = t('serialLog.actions.connect');
  }

  let buttonVariant: 'primary' | 'danger' | 'secondary' = 'primary';
  if (status === 'connected') {
    buttonVariant = 'danger';
  } else if (isTransitioning) {
    buttonVariant = 'secondary';
  }

  return (
    <StyledButton
      onClick={() => {
        void (status === 'connected' ? disconnect() : connect());
      }}
      variant={buttonVariant}
      disabled={isTransitioning}
    >
      {buttonText}
    </StyledButton>
  );
};
