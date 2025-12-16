import { useState } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { type Mode } from '@/app/models/mode';
import { useSerialStore } from '@/app/providers/serial-store';

import { FlashModal } from './FlashModal';
import { StyledButton } from './StyledButton';

interface SerialFlashButtonProps {
  mode: Mode;
  disabled: boolean;
}

export const SerialFlashButton = ({ mode, disabled }: SerialFlashButtonProps) => {
  const { t } = useTranslation();
  const status = useSerialStore(s => s.status);
  const send = useSerialStore(s => s.send);
  const [isModalOpen, setIsModalOpen] = useState(false);

  const handleFlash = async (index: number) => {
    if (status !== 'connected' || disabled) return;
    const flashCommand = {
      "command": "writeMode",
      "index": index,
      mode,
    }

    try {
      await send(flashCommand);
      toast.success(t('common.actions.flashSuccess'));
      setIsModalOpen(false);
    } catch (err) {
      console.error('Failed to flash mode', err);
      toast.error(t('common.actions.flashError'));
    }
  };

  if (status !== 'connected') return null;

  return (
    <>
      <StyledButton 
        onClick={() => { setIsModalOpen(true); }} 
        variant="primary"
        disabled={disabled}
        title={t('common.hints.fixValidationErrors')}
      >
        {t('common.actions.flash')}
      </StyledButton>
      <FlashModal
        isOpen={isModalOpen}
        onClose={() => { setIsModalOpen(false); }}
        onConfirm={(index) => void handleFlash(index)}
      />
    </>
  );
};
