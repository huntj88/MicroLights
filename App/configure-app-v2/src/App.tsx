import { useTranslation } from 'react-i18next';
import { HashRouter, Routes, Route, Navigate } from 'react-router-dom';

import { AppShell } from './components/AppShell';
import CreateMode from './pages/CreateMode';
import CreateWave from './pages/CreateWave';
import SerialLog from './pages/SerialLog';
import Settings from './pages/Settings';

function Placeholder({ title }: { title: string }) {
  const { t } = useTranslation();
  return (
    <div className="space-y-2">
      <h1 className="text-2xl font-semibold">{title}</h1>
      <p className="text-fg-muted">{t('workInProgress')}</p>
    </div>
  );
}

export function App() {
  const { t } = useTranslation();
  return (
    <HashRouter>
      <Routes>
        <Route element={<AppShell />}>
          <Route index element={<Navigate to="/create/mode" replace />} />
          <Route path="create">
            <Route path="mode" element={<CreateMode />} />
            <Route path="wave" element={<CreateWave />} />
          </Route>
          <Route path="browse" element={<Placeholder title={t('browseTitle')} />} />
          <Route path="program" element={<Placeholder title={t('programTitle')} />} />
          <Route path="settings" element={<Settings />} />
          <Route path="extras">
            <Route path="serial-log" element={<SerialLog />} />
          </Route>
          <Route path="docs" element={<Placeholder title={t('docsTitle')} />} />
          <Route path="examples" element={<Placeholder title={t('examplesTitle')} />} />
          <Route path="*" element={<Navigate to="/create/mode" replace />} />
        </Route>
      </Routes>
    </HashRouter>
  );
}
