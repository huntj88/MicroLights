import { useTranslation } from 'react-i18next';
import { NavLink, Outlet } from 'react-router-dom';

import { ROUTES, routeOrder } from '../router/constants';

const resolvePath = (key: keyof typeof ROUTES) =>
  ROUTES[key] === '/' ? ROUTES[key] : `/${ROUTES[key]}`;

export const AppLayout = () => {
  const { t } = useTranslation();
  const currentYear = new Date().getFullYear();

  return (
    <div className="theme-surface flex min-h-screen flex-col">
      <a className="sr-only focus:not-sr-only focus:absolute focus:top-4 focus:left-4" href="#main">
        {t('layout.skipToContent')}
      </a>
  <header className="border-b border-solid theme-border backdrop-blur">
        <div className="mx-auto flex max-w-6xl items-center justify-between gap-4 px-4 py-4">
          <div>
            <p className="text-xs uppercase tracking-widest theme-muted">{t('app.tagline')}</p>
            <h1 className="text-xl font-semibold">{t('app.name')}</h1>
          </div>
          <nav aria-label={t('app.name')} className="flex items-center gap-3 text-sm">
            {routeOrder.map(routeKey => {
              const href = resolvePath(routeKey);
              return (
                <NavLink
                  key={routeKey}
                  className={({ isActive }) =>
                    [
                      'rounded-full px-4 py-2 transition-colors',
                      isActive
                        ? 'bg-[rgb(var(--accent)/1)] text-[rgb(var(--surface-contrast)/1)]'
                        : 'theme-muted hover:text-[rgb(var(--surface-contrast)/1)]',
                    ].join(' ')
                  }
                  to={href}
                  end={routeKey === 'home'}
                >
                  {t(`nav.${routeKey}`)}
                </NavLink>
              );
            })}
          </nav>
        </div>
      </header>
      <main className="mx-auto w-full max-w-6xl flex-1 px-4 py-8" id="main">
        <Outlet />
      </main>
      <footer className="border-t border-solid theme-border">
        <div className="mx-auto max-w-6xl px-4 py-4 text-sm theme-muted">
          {t('layout.footer', { year: currentYear })}
        </div>
      </footer>
    </div>
  );
};
