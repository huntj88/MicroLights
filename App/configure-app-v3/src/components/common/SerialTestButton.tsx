import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { type Mode, type ModePattern } from '@/app/models/mode';
import { useSerialStore } from '@/app/providers/serial-store';

import { StyledButton } from './StyledButton';

interface SerialTestButtonProps {
  data: Mode | ModePattern;
  type: 'mode' | 'pattern';
  patternTarget?: 'front' | 'case';
  disabled: boolean;
}

export const SerialTestButton = ({ data, type, patternTarget, disabled }: SerialTestButtonProps) => {
  const { t } = useTranslation();
  const status = useSerialStore(s => s.status);
  const send = useSerialStore(s => s.send);

  const handleTest = async () => {
    if (status !== 'connected' || disabled) return;

    let payload: Mode;

    if (type === 'pattern') {
      if (!patternTarget) {
        console.error('patternTarget is required when testing a pattern');
        return;
      }
      payload = {
        name: 'transientTest',
        [patternTarget]: { pattern: data as ModePattern },
      };
    } else {
      // Mode
      payload = { ...(data as Mode), name: 'transientTest' };
    }

    try {
      await send(payload);
      toast.success(t('common.actions.testSuccess'));
    } catch (err) {
      console.error('Failed to send test data', err);
      toast.error(t('common.actions.testError'));
    }
  };

  if (status !== 'connected') return null;

  return (
    <StyledButton onClick={() => void handleTest()} variant="secondary" disabled={disabled} title={t('common.hints.fixValidationErrors')}>
      {t('common.actions.test')}
    </StyledButton>
  );
};
