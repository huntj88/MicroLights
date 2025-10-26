import { useTranslation } from 'react-i18next';

export const BulbPatternPage = () => {
  const { t } = useTranslation();

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('bulbPattern.title')}</h2>
        <p className="theme-muted">{t('bulbPattern.subtitle')}</p>
      </header>
      <div className="theme-panel theme-border rounded-xl border p-6">
        <p className="theme-muted">{t('bulbPattern.todo')}</p>
      </div>
    </section>
  );
};
