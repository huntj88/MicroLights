import { HashRouter, Navigate, Route, Routes } from 'react-router-dom';

import { BulbPatternPage } from '@/pages/bulb-pattern/BulbPatternPage';
import { HomePage } from '@/pages/home/HomePage';
import { ModePage } from '@/pages/mode/ModePage';
import { RgbPatternPage } from '@/pages/rgb-pattern/RgbPatternPage';
import { SerialLogPage } from '@/pages/serial-log/SerialLogPage';
import { SettingsPage } from '@/pages/settings/SettingsPage';

import { ROUTES } from './constants';
import { AppLayout } from '../layout/AppLayout';

export const AppRouter = () => (
  <HashRouter>
    <Routes>
      <Route element={<AppLayout />} path={ROUTES.home}>
        <Route index element={<HomePage />} />
        <Route element={<RgbPatternPage />} path={ROUTES.rgbPattern} />
        <Route element={<BulbPatternPage />} path={ROUTES.bulbPattern} />
        <Route element={<ModePage />} path={ROUTES.mode} />
        <Route element={<SerialLogPage />} path={ROUTES.serialLog} />
        <Route element={<SettingsPage />} path={ROUTES.settings} />
        <Route element={<Navigate replace to={ROUTES.home} />} path="*" />
      </Route>
    </Routes>
  </HashRouter>
);
