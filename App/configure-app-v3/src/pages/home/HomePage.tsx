import { useTranslation } from 'react-i18next';

export const HomePage = () => {
  const { t } = useTranslation();

  return (
    <section className="space-y-6">
      <header className="space-y-2">
        <h2 className="text-3xl font-semibold">{t('home.title')}</h2>
        <p className="max-w-2xl theme-muted">{t('home.subtitle')}</p>
      </header>
      <p className="max-w-2xl theme-muted">{t('home.description')}</p>
    </section>
  );
};
