import { useCallback, useEffect, useState } from 'react';
import toast from 'react-hot-toast';
import { useTranslation } from 'react-i18next';

import { type Mode, type ModePattern } from '@/app/models/mode';
import { useSerialStore } from '@/app/providers/serial-store';

import { StyledButton } from './StyledButton';

interface SerialTestButtonProps {
  data: Mode | ModePattern;
  type: 'mode' | 'pattern';
  patternTarget?: 'front' | 'case'; // Required if type is 'pattern'
  disabled: boolean;
}

export const SerialTestButton = ({
  data,
  type,
  patternTarget,
  disabled,
}: SerialTestButtonProps) => {
  const { t } = useTranslation();
  const status = useSerialStore(s => s.status);
  const send = useSerialStore(s => s.send);
  const [isAutoSync, setIsAutoSync] = useState(false);

  // Reset auto-sync if disconnected or disabled
  useEffect(() => {
    if (status !== 'connected' || disabled) {
      setIsAutoSync(false);
    }
  }, [status, disabled]);

  const handleTest = useCallback(
    async (silent = false) => {
      if (status !== 'connected' || disabled) return;

      let mode: Mode;

      if (type === 'pattern') {
        if (!patternTarget) {
          console.error('patternTarget is required when testing a pattern');
          return;
        }
        mode = {
          name: 'transientTest',
          [patternTarget]: { pattern: data as ModePattern },
        };
      } else {
        // Mode
        mode = { ...(data as Mode), name: 'transientTest' };
      }

      const command = {
        command: 'writeMode',
        index: 0,
        mode,
      };

      try {
        await send(command);
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
    [status, disabled, type, patternTarget, data, send, t],
  );

  // Auto-sync effect
  useEffect(() => {
    if (!isAutoSync) return;

    const timer = setTimeout(() => {
      void handleTest(true);
    }, 100); // Debounce slightly to avoid flooding

    return () => {
      clearTimeout(timer);
    };
  }, [data, isAutoSync, handleTest]);

  if (status !== 'connected') return null;

  return (
    <StyledButton
      onClick={() => {
        if (isAutoSync) {
          setIsAutoSync(false);
        } else {
          setIsAutoSync(true);
          void handleTest(false);
        }
      }}
      variant={isAutoSync ? 'primary' : 'secondary'}
      disabled={disabled}
      title={t('common.hints.fixValidationErrors')}
    >
      {isAutoSync ? t('common.actions.stopTest') : t('common.actions.test')}
    </StyledButton>
  );
};
