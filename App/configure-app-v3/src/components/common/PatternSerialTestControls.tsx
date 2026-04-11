import { useEffect, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { type Mode, type ModePattern } from '@/app/models/mode';

import { StyledButton } from './StyledButton';
import { useSerialTestSender } from './useSerialTestSender';

type PatternTarget = 'front' | 'case';

interface PatternSerialTestControlsProps {
  data: ModePattern;
  disabled: boolean;
}

const patternTargets: PatternTarget[] = ['front', 'case'];

export const PatternSerialTestControls = ({
  data,
  disabled,
}: PatternSerialTestControlsProps) => {
  const { t } = useTranslation();
  const { status, sendTestMode } = useSerialTestSender();
  const [activeTargets, setActiveTargets] = useState<PatternTarget[]>([]);

  useEffect(() => {
    if (status !== 'connected') {
      setActiveTargets([]);
    }
  }, [status]);

  useEffect(() => {
    if (activeTargets.length === 0 || disabled) return;

    const timer = setTimeout(() => {
      void sendTestMode(buildPatternPreviewMode(data, activeTargets), true);
    }, 100);

    return () => {
      clearTimeout(timer);
    };
  }, [activeTargets, data, disabled, sendTestMode]);

  if (status !== 'connected') return null;

  return (
    <>
      {patternTargets.map(target => {
        const isActive = activeTargets.includes(target);
        const isButtonDisabled = disabled && !isActive;

        return (
          <StyledButton
            key={target}
            onClick={() => {
              const nextTargets = activeTargets.includes(target)
                ? activeTargets.filter(activeTarget => activeTarget !== target)
                : patternTargets.filter(
                    candidate => candidate === target || activeTargets.includes(candidate),
                  );

              setActiveTargets(nextTargets);

              if (nextTargets.length > 0) {
                void sendTestMode(buildPatternPreviewMode(data, nextTargets));
              }
            }}
            variant={isActive ? 'primary' : 'secondary'}
            disabled={isButtonDisabled}
            title={isButtonDisabled ? t('common.hints.fixValidationErrors') : undefined}
          >
            {getPatternButtonLabel(target, isActive, t)}
          </StyledButton>
        );
      })}
    </>
  );
};

const buildPatternPreviewMode = (data: ModePattern, previewTargets: PatternTarget[]): Mode => ({
  name: 'transientTest',
  ...Object.fromEntries(previewTargets.map(target => [target, { pattern: data }])),
});

const getPatternButtonLabel = (
  target: PatternTarget,
  isActive: boolean,
  t: ReturnType<typeof useTranslation>['t'],
) => {
  if (target === 'front') {
    return isActive ? t('common.actions.stopTestFront') : t('common.actions.testFront');
  }

  return isActive ? t('common.actions.stopTestCase') : t('common.actions.testCase');
};