import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import { Modal } from './Modal';
import { StyledButton } from './StyledButton';

interface FlashModalProps {
  isOpen: boolean;
  onClose: () => void;
  onConfirm: (index: number) => void;
}

export const FlashModal = ({ isOpen, onClose, onConfirm }: FlashModalProps) => {
  const { t } = useTranslation();
  const [index, setIndex] = useState(0);

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={t('common.actions.flash')} maxWidth="md">
      <h2 className="mb-4 text-xl font-bold text-[rgb(var(--surface-contrast)/1)]">
        {t('common.actions.flash')}
      </h2>

      <div className="mb-6">
        <label className="mb-2 block text-sm font-medium theme-muted">
          {t('common.labels.index')}
        </label>
        <div className="grid grid-cols-3 gap-2 sm:grid-cols-6">
          {[0, 1, 2, 3, 4, 5].map(i => (
            <StyledButton
              key={i}
              onClick={() => {
                setIndex(i);
              }}
              variant={index === i ? 'primary' : 'secondary'}
              className="w-full justify-center"
            >
              {i + 1}
            </StyledButton>
          ))}
        </div>
        <p className="mt-2 text-sm theme-muted">{t('common.hints.flashIndex')}</p>
      </div>

      <div className="flex justify-end space-x-3">
        <StyledButton onClick={onClose} variant="secondary">
          {t('common.actions.cancel')}
        </StyledButton>
        <StyledButton
          onClick={() => {
            onConfirm(index);
          }}
          variant="primary"
        >
          {t('common.actions.flash')}
        </StyledButton>
      </div>
    </Modal>
  );
};
