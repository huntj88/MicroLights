import { useCallback, useEffect, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { NavLink, useLocation } from 'react-router-dom';

import { LayersIcon, PaletteIcon, RadioIcon } from '../../components/icons/NavIcons';
import { ROUTES } from '../router/constants';

const PATTERN_STORAGE_KEY = 'last-pattern-route';

const tabs = [
  {
    labelKey: 'nav.patterns' as const,
    icon: PaletteIcon,
    getPath: (lastPattern: string) => `/${lastPattern}`,
    isActive: (pathname: string) => pathname.startsWith('/patterns'),
  },
  {
    labelKey: 'nav.mode' as const,
    icon: LayersIcon,
    getPath: () => `/${ROUTES.mode}`,
    isActive: (pathname: string) => pathname.startsWith(`/${ROUTES.mode}`),
  },
  {
    labelKey: 'nav.serialLog' as const,
    icon: RadioIcon,
    getPath: () => `/${ROUTES.serialLog}`,
    isActive: (pathname: string) => pathname.startsWith(`/${ROUTES.serialLog}`),
  },
];

export const MobileBottomNav = () => {
  const { t } = useTranslation();
  const location = useLocation();

  const [lastPatternRoute, setLastPatternRoute] = useState<string>(() => {
    try {
      return sessionStorage.getItem(PATTERN_STORAGE_KEY) ?? ROUTES.rgbPattern;
    } catch {
      return ROUTES.rgbPattern;
    }
  });

  // Track last-visited pattern sub-route
  useEffect(() => {
    const path = location.pathname.slice(1); // remove leading /
    if (path === ROUTES.rgbPattern || path === ROUTES.bulbPattern) {
      setLastPatternRoute(path);
      try {
        sessionStorage.setItem(PATTERN_STORAGE_KEY, path);
      } catch {
        // ignore
      }
    }
  }, [location.pathname]);

  const resolvePatternPath = useCallback(() => {
    return `/${lastPatternRoute}`;
  }, [lastPatternRoute]);

  return (
    <nav
      role="navigation"
      aria-label={t('nav.menu')}
      className="fixed bottom-0 inset-x-0 z-30 md:hidden border-t theme-border bg-[rgb(var(--surface)/0.95)] backdrop-blur-md pb-[env(safe-area-inset-bottom)]"
    >
      <div className="flex items-center justify-around">
        {tabs.map(tab => {
          const Icon = tab.icon;
          const isActive = tab.isActive(location.pathname);
          const path = tab.labelKey === 'nav.patterns' ? resolvePatternPath() : tab.getPath();

          return (
            <NavLink
              key={tab.labelKey}
              to={path}
              aria-current={isActive ? 'page' : undefined}
              className={`flex flex-col items-center justify-center gap-1 min-h-[56px] min-w-[44px] flex-1 py-2 transition-colors ${
                isActive
                  ? 'text-[rgb(var(--accent)/1)]'
                  : 'theme-muted hover:text-[rgb(var(--surface-contrast)/1)]'
              }`}
            >
              <Icon className="h-5 w-5" />
              <span className="text-xs font-medium">{t(tab.labelKey)}</span>
            </NavLink>
          );
        })}
      </div>
    </nav>
  );
};
