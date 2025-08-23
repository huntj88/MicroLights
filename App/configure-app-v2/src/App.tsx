import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';

import { AppShell } from './components/AppShell';
import CreateSet from './pages/CreateSet';

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
          <Route index element={<Navigate to="/create/set" replace />} />
          <Route path="create">
            <Route path="set" element={<CreateSet />} />
            <Route path="wave" element={<Placeholder title="Create / Wave" />} />
          </Route>
          <Route path="browse" element={<Placeholder title="Browse" />} />
          <Route path="program" element={<Placeholder title="Program" />} />
          <Route path="theme" element={<Placeholder title="Theme" />} />
          <Route path="docs" element={<Placeholder title="Docs" />} />
          <Route path="examples" element={<Placeholder title="Examples" />} />
          <Route path="*" element={<Navigate to="/create/set" replace />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}
