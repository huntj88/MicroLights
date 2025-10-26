import i18n from 'i18next';
import LanguageDetector from 'i18next-browser-languagedetector';
import { initReactI18next } from 'react-i18next';

import enCommon from '@/locales/en/common.json';

void i18n
  .use(LanguageDetector)
  .use(initReactI18next)
  .init({
    detection: {
      order: ['localStorage', 'navigator', 'htmlTag'],
      caches: ['localStorage']
    },
    fallbackLng: 'en',
    supportedLngs: ['en'],
    defaultNS: 'common',
    resources: {
      en: {
        common: enCommon
      }
    },
    interpolation: {
      escapeValue: false
    },
    react: {
      useSuspense: true
    }
  });

export { i18n };
