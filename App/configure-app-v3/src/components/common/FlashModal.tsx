import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import { StyledButton } from './StyledButton';

interface FlashModalProps {
  isOpen: boolean;
  onClose: () => void;
  onConfirm: (index: number) => void;
}

export const FlashModal = ({ isOpen, onClose, onConfirm }: FlashModalProps) => {
  const { t } = useTranslation();
  const [index, setIndex] = useState(0);

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50">
      <div className="w-full max-w-md rounded-lg bg-white p-6 shadow-xl dark:bg-gray-800">
        <h2 className="mb-4 text-xl font-bold text-gray-900 dark:text-white">
          {t('common.actions.flash')}
        </h2>

        <div className="mb-6">
          <label className="mb-2 block text-sm font-medium text-gray-700 dark:text-gray-300">
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
          <p className="mt-2 text-sm text-gray-500 dark:text-gray-400">
            {t('common.hints.flashIndex')}
          </p>
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
      </div>
    </div>
  );
};
