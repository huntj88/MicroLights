import { useTranslation } from 'react-i18next';
import { NavLink, useLocation } from 'react-router-dom';

import { ROUTES } from '../../app/router/constants';

/**
 * Segmented control for switching between RGB and Bulb pattern pages.
 * Only shown on mobile (hidden at md+) since desktop has full nav links.
 */
export const PatternTypeSwitcher = () => {
  const { t } = useTranslation();
  const location = useLocation();

  const isRgb = location.pathname.includes(ROUTES.rgbPattern);

  return (
    <nav
      className="flex md:hidden overflow-hidden rounded-full border theme-border"
      aria-label={t('nav.patterns')}
    >
      <NavLink
        to={`/${ROUTES.rgbPattern}`}
        className={`flex-1 text-center px-4 py-2 text-sm font-medium transition-colors min-h-[44px] flex items-center justify-center ${
          isRgb
            ? 'bg-[rgb(var(--accent)/0.25)] text-[rgb(var(--accent-contrast)/1)]'
            : 'theme-muted hover:bg-[rgb(var(--surface-raised)/0.5)]'
        }`}
      >
        {t('nav.rgbPattern')}
      </NavLink>
      <NavLink
        to={`/${ROUTES.bulbPattern}`}
        className={`flex-1 text-center px-4 py-2 text-sm font-medium transition-colors min-h-[44px] flex items-center justify-center ${
          !isRgb
            ? 'bg-[rgb(var(--accent)/0.25)] text-[rgb(var(--accent-contrast)/1)]'
            : 'theme-muted hover:bg-[rgb(var(--surface-raised)/0.5)]'
        }`}
      >
        {t('nav.bulbPattern')}
      </NavLink>
    </nav>
  );
};
