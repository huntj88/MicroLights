import { useCallback } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { type Mode } from '@/app/models/mode';
import { useSerialStore } from '@/app/providers/serial-store';

export const useSerialTestSender = () => {
  const { t } = useTranslation();
  const status = useSerialStore(s => s.status);
  const send = useSerialStore(s => s.send);

  const sendTestMode = useCallback(
    async (mode: Mode, silent = false) => {
      if (status !== 'connected') return;

      try {
        await send({
          command: 'writeMode',
          index: 0,
          mode,
        });

        if (!silent) {
          toast.success(t('common.actions.testSuccess'));
        }
      } catch (err) {
        console.error('Failed to send test data', err);
        if (!silent) {
          toast.error(t('common.actions.testError'));
        }
      }
    },
    [status, send, t],
  );

  return {
    status,
    sendTestMode,
  };
};