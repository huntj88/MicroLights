import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';

import { AppShell } from './components/AppShell';
import CreateMode from './pages/CreateMode';
import CreateWave from './pages/CreateWave';
import Settings from './pages/Settings';

function Placeholder({ title }: { title: string }) {
  return (
    <div className="space-y-2">
      <h1 className="text-2xl font-semibold">{title}</h1>
      <p className="text-fg-muted">Work in progress.</p>
    </div>
  );
}

export function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route element={<AppShell />}>
          <Route index element={<Navigate to="/create/mode" replace />} />
          <Route path="create">
            <Route path="mode" element={<CreateMode />} />
            <Route path="wave" element={<CreateWave />} />
          </Route>
          <Route path="browse" element={<Placeholder title="Browse" />} />
          <Route path="program" element={<Placeholder title="Program" />} />
          <Route path="settings" element={<Settings />} />
          <Route path="docs" element={<Placeholder title="Docs" />} />
          <Route path="examples" element={<Placeholder title="Examples" />} />
          <Route path="*" element={<Navigate to="/create/mode" replace />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}
