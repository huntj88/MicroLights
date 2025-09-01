import { useEffect, useMemo, useRef, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { serial, type SerialLogEntry } from '@/lib/serial';
import { useAppStore } from '@/lib/store';

export default function SerialLog() {
  const { t } = useTranslation();
  const [entries, setEntries] = useState<ReadonlyArray<SerialLogEntry>>(() => serial.getLog());
  const [autoScroll, setAutoScroll] = useState(true);
  const containerRef = useRef<HTMLDivElement | null>(null);
  const connected = useAppStore(s => s.connected);
  const connect = useAppStore(s => s.connect);
  const disconnect = useAppStore(s => s.disconnect);

  useEffect(() => {
    const off = serial.on('log', () => {
      setEntries([...serial.getLog()]);
    });
    return () => off();
  }, []);

  const rows = useMemo(() => entries.slice(-500), [entries]);

  useEffect(() => {
    if (!autoScroll) return;
    const el = containerRef.current;
    if (!el) return;
    el.scrollTop = el.scrollHeight;
  }, [rows, autoScroll]);

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold">{t('serialLogTitle')}</h1>
        <div className="flex items-center gap-2">
          <label className="text-sm flex items-center gap-2">
            <input
              type="checkbox"
              checked={autoScroll}
              onChange={e => setAutoScroll(e.target.checked)}
            />
            {t('autoScroll')}
          </label>
          <button
            type="button"
            className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white"
            onClick={() => {
              serial.clearLog();
              setEntries([]);
            }}
          >
            {t('clear')}
          </button>
          <button
            type="button"
            onClick={() => (connected ? disconnect() : connect())}
            className="px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white"
          >
            {connected ? t('disconnect') : t('connect')}
          </button>
        </div>
      </div>

      <div
        ref={containerRef}
        className="rounded border border-slate-700/50 bg-slate-900/60 p-2 text-xs h-[60vh] overflow-auto"
        id="serial-log-container"
      >
        {rows.length === 0 ? (
          <div className="text-slate-400">{t('noSerialData')}</div>
        ) : (
          <table className="w-full text-left text-slate-200">
            <thead className="text-slate-400">
              <tr>
                <th className="px-2 py-1">{t('time')}</th>
                <th className="px-2 py-1">{t('direction')}</th>
                <th className="px-2 py-1">{t('status')}</th>
                <th className="px-2 py-1">{t('payload')}</th>
              </tr>
            </thead>
            <tbody>
              {rows.map(e => (
                <tr key={e.id} className="border-t border-slate-700/40">
                  <td className="px-2 py-1 whitespace-nowrap align-top">
                    {new Date(e.ts).toLocaleTimeString()}
                  </td>
                  <td className="px-2 py-1 whitespace-nowrap align-top">
                    <span
                      className={
                        e.dir === 'tx'
                          ? 'text-emerald-400 uppercase text-[10px] tracking-wide'
                          : 'text-sky-400 uppercase text-[10px] tracking-wide'
                      }
                    >
                      {e.dir === 'tx' ? t('tx') : t('rx')}
                    </span>
                  </td>
                  <td className="px-2 py-1 whitespace-nowrap align-top">
                    {e.ok ? (
                      <span className="text-emerald-400">{t('ok')}</span>
                    ) : (
                      <span className="text-red-400" title={e.error}>
                        {t('error')}
                      </span>
                    )}
                  </td>
                  <td className="px-2 py-1 align-top">
                    <pre className="whitespace-pre-wrap break-words">
                      {e.ok ? JSON.stringify(e.json) : e.raw}
                    </pre>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  );
}
