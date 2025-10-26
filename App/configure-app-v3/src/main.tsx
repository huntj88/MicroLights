import { StrictMode, Suspense } from 'react';
import { createRoot } from 'react-dom/client';
import { I18nextProvider } from 'react-i18next';

import { i18n } from './app/providers/i18n';
import { ThemeProvider } from './app/providers/ThemeProvider';
import { App } from './App.tsx';
import './index.css';

const rootElement = document.getElementById('root');

if (!rootElement) {
  throw new Error('Root element not found');
}

createRoot(rootElement).render(
  <StrictMode>
    <I18nextProvider i18n={i18n}>
      <ThemeProvider>
        <Suspense fallback={<div className="p-6 text-neutral-200">Loading…</div>}>
          <App />
        </Suspense>
      </ThemeProvider>
    </I18nextProvider>
  </StrictMode>,
);
