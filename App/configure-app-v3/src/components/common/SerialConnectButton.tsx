import { useTranslation } from 'react-i18next';

import { useSerialStore } from '@/app/providers/serial-store';

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
  );
};
