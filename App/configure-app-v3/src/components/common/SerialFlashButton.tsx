import { useTranslation } from 'react-i18next';

import { type Mode } from '@/app/models/mode';
import { useSerialStore } from '@/app/providers/serial-store';

interface SerialFlashButtonProps {
  mode: Mode;
}

export const SerialFlashButton = ({ mode }: SerialFlashButtonProps) => {
  const { t } = useTranslation();
  const status = useSerialStore(s => s.status);
  const send = useSerialStore(s => s.send);

  const handleFlash = async () => {
    if (status !== 'connected') return;

    try {
      await send(mode);
    } catch (err) {
      console.error('Failed to flash mode', err);
    }
  };

  if (status !== 'connected') return null;

  return (
    <button
      onClick={() => void handleFlash()}
      className="rounded-full bg-purple-100 px-4 py-2 text-sm font-medium text-purple-700 transition-colors hover:bg-purple-200 dark:bg-purple-900/30 dark:text-purple-300"
    >
      {t('common.actions.flash')}
    </button>
  );
};
