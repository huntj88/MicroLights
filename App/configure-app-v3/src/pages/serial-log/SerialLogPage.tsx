import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import {
  SerialLogPanel,
  type SerialLogPanelProps,
  type SerialLogState,
} from '@/components/serial-log/SerialLogPanel';

export const SerialLogPage = () => {
  const { t } = useTranslation();
  const [state, setState] = useState<SerialLogState>({
    autoscroll: true,
    entries: [],
    pendingPayload: '',
  });

  const handleChange: SerialLogPanelProps['onChange'] = newState => {
    setState(newState);
  };

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('serialLog.title')}</h2>
        <p className="theme-muted">{t('serialLog.subtitle')}</p>
      </header>
      <SerialLogPanel onChange={handleChange} value={state} />
      <p className="theme-muted text-sm">{t('serialLog.todo')}</p>
    </section>
  );
};
