import { useEffect, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { type Mode } from '@/app/models/mode';

import { StyledButton } from './StyledButton';
import { useSerialTestSender } from './useSerialTestSender';

interface ModeSerialTestButtonProps {
  data: Mode;
  disabled: boolean;
}

export const ModeSerialTestButton = ({ data, disabled }: ModeSerialTestButtonProps) => {
  const { t } = useTranslation();
  const { status, sendTestMode } = useSerialTestSender();
  const [isAutoSync, setIsAutoSync] = useState(false);

  useEffect(() => {
    if (status !== 'connected') {
      setIsAutoSync(false);
    }
  }, [status]);

  useEffect(() => {
    if (!isAutoSync || disabled) return;

    const timer = setTimeout(() => {
      void sendTestMode({ ...data, name: 'transientTest' }, true);
    }, 100);

    return () => {
      clearTimeout(timer);
    };
  }, [data, disabled, isAutoSync, sendTestMode]);

  if (status !== 'connected') return null;

  const isButtonDisabled = disabled && !isAutoSync;

  return (
    <StyledButton
      onClick={() => {
        if (isAutoSync) {
          setIsAutoSync(false);
          return;
        }

        setIsAutoSync(true);
        void sendTestMode({ ...data, name: 'transientTest' });
      }}
      variant={isAutoSync ? 'primary' : 'secondary'}
      disabled={isButtonDisabled}
      title={isButtonDisabled ? t('common.hints.fixValidationErrors') : undefined}
    >
      {isAutoSync ? t('common.actions.stopTest') : t('common.actions.test')}
    </StyledButton>
  );
};