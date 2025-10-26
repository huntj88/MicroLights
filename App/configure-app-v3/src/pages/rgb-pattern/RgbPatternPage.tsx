import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import type { ModePattern } from '../../app/models/mode';
import {
  type SimpleRgbPatternAction,
  SimpleRgbPatternPanel,
} from '../../components/rgb-pattern/SimpleRgbPatternPanel';

export const RgbPatternPage = () => {
  const { t } = useTranslation();
  const [simplePatternState, setSimplePatternState] = useState<ModePattern>(() => ({
    name: 'Simple RGB Pattern',
    duration: 0,
    changeAt: [],
  }));

  const handleSimplePatternChange = (nextPattern: ModePattern, action: SimpleRgbPatternAction) => {
    void action;
    setSimplePatternState(nextPattern);
  };

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('rgbPattern.title')}</h2>
        <p className="theme-muted">{t('rgbPattern.subtitle')}</p>
      </header>
      <article className="space-y-6 rounded-2xl border border-dashed border-white/10 bg-[rgb(var(--surface-raised)/0.35)] p-6">
        <header className="space-y-1">
          <h3 className="text-2xl font-semibold">{t('rgbPattern.simple.title')}</h3>
          <p className="theme-muted text-sm">{t('rgbPattern.simple.description')}</p>
        </header>
  <SimpleRgbPatternPanel onChange={handleSimplePatternChange} value={simplePatternState} />
      </article>
      <article className="theme-panel theme-border rounded-2xl border p-6">
        <h3 className="text-2xl font-semibold">{t('rgbPattern.equation.title')}</h3>
        <p className="theme-muted">{t('rgbPattern.equation.todo')}</p>
      </article>
    </section>
  );
};
