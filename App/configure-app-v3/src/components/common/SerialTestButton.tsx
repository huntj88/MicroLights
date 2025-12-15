import { useTranslation } from 'react-i18next';

import { type Mode, type ModePattern } from '@/app/models/mode';
import { useSerialStore } from '@/app/providers/serial-store';

import { StyledButton } from './StyledButton';

interface SerialTestButtonProps {
  data: Mode | ModePattern;
  type: 'mode' | 'pattern';
  patternTarget?: 'front' | 'case';
}

export const SerialTestButton = ({ data, type, patternTarget }: SerialTestButtonProps) => {
  const { t } = useTranslation();
  const status = useSerialStore(s => s.status);
  const send = useSerialStore(s => s.send);

  const handleTest = async () => {
    if (status !== 'connected') return;

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
    } catch (err) {
      console.error('Failed to send test data', err);
    }
  };

  if (status !== 'connected') return null;

  return (
    <StyledButton onClick={() => void handleTest()} variant="secondary">
      {t('common.actions.test')}
    </StyledButton>
  );
};
