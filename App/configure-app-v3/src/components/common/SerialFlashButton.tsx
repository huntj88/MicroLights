import { useTranslation } from 'react-i18next';

import { type Mode } from '@/app/models/mode';
import { useSerialStore } from '@/app/providers/serial-store';

import { StyledButton } from './StyledButton';

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
    <StyledButton onClick={() => void handleFlash()} variant="primary">
      {t('common.actions.flash')}
    </StyledButton>
  );
};
