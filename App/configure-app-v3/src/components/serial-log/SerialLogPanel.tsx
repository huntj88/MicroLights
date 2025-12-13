import type { ChangeEvent, FormEvent } from 'react';
import { useEffect, useMemo, useRef } from 'react';
import { useTranslation } from 'react-i18next';

export type SerialLogDirection = 'inbound' | 'outbound';

export interface SerialLogEntry {
  id: string;
  timestamp: string;
  direction: SerialLogDirection;
  payload: string;
}

export interface SerialLogState {
  entries: SerialLogEntry[];
  autoscroll: boolean;
  pendingPayload: string;
}

export type SerialLogAction =
  | { type: 'toggle-autoscroll'; autoscroll: boolean }
  | { type: 'clear' }
  | { type: 'update-payload'; value: string }
  | { type: 'submit'; entry: SerialLogEntry };

export interface SerialLogPanelProps {
  value: SerialLogState;
  onChange: (newState: SerialLogState, action: SerialLogAction) => void;
}

const timestampFormatter = new Intl.DateTimeFormat(undefined, {
  hour: '2-digit',
  minute: '2-digit',
  second: '2-digit',
  fractionalSecondDigits: 3,
});

const createEntry = (payload: string, direction: SerialLogDirection): SerialLogEntry => ({
  id: `${Date.now().toString(36)}-${Math.random().toString(16).slice(2)}`,
  timestamp: new Date().toISOString(),
  direction,
  payload,
});

export const SerialLogPanel = ({ value, onChange }: SerialLogPanelProps) => {
  const { t } = useTranslation();
  const viewportRef = useRef<HTMLDivElement | null>(null);

  useEffect(() => {
    if (!value.autoscroll || !viewportRef.current) {
      return;
    }

    if (typeof viewportRef.current.scrollTo === 'function') {
      viewportRef.current.scrollTo({ top: viewportRef.current.scrollHeight, behavior: 'smooth' });
      return;
    }

    viewportRef.current.scrollTop = viewportRef.current.scrollHeight;
  }, [value.autoscroll, value.entries]);

  const handlePayloadChange = (event: ChangeEvent<HTMLTextAreaElement>) => {
    const nextValue = event.target.value;
    onChange(
      {
        ...value,
        pendingPayload: nextValue,
      },
      { type: 'update-payload', value: nextValue },
    );
  };

  const handleAutoscrollToggle = () => {
    const nextAutoscroll = !value.autoscroll;
    onChange(
      {
        ...value,
        autoscroll: nextAutoscroll,
      },
      { type: 'toggle-autoscroll', autoscroll: nextAutoscroll },
    );
  };

  const handleClear = () => {
    if (value.entries.length === 0) {
      return;
    }

    onChange(
      {
        ...value,
        entries: [],
      },
      { type: 'clear' },
    );
  };

  const handleSubmit = (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    const trimmedPayload = value.pendingPayload.trim();

    if (!trimmedPayload) {
      return;
    }

    const entry = createEntry(trimmedPayload, 'outbound');

    onChange(
      {
        autoscroll: value.autoscroll,
        entries: [...value.entries, entry],
        pendingPayload: '',
      },
      { type: 'submit', entry },
    );
  };

  const renderedEntries = useMemo(
    () =>
      value.entries.map(entry => (
        <li
          key={entry.id}
          className="flex flex-col gap-1 border-b border-dashed border-white/5 pb-2 last:border-none last:pb-0"
        >
          <span className="font-mono text-xs theme-muted">
            {timestampFormatter.format(new Date(entry.timestamp))} ·{' '}
            {t(`serialLog.direction.${entry.direction}`)}
          </span>
          <code className="whitespace-pre-wrap break-words text-sm">{entry.payload}</code>
        </li>
      )),
    [t, value.entries],
  );

  return (
    <div className="space-y-6">
      <div className="flex flex-wrap items-center gap-2">
        <button
          className="rounded-full border border-solid theme-border px-3 py-1 text-sm transition-colors hover:bg-[rgb(var(--surface-raised)/1)]"
          onClick={handleAutoscrollToggle}
          type="button"
          aria-pressed={value.autoscroll}
        >
          {value.autoscroll
            ? t('serialLog.actions.autoscrollOn')
            : t('serialLog.actions.autoscrollOff')}
        </button>
        <button
          className="rounded-full border border-solid theme-border px-3 py-1 text-sm transition-colors hover:bg-[rgb(var(--surface-raised)/1)]"
          disabled={value.entries.length === 0}
          onClick={handleClear}
          type="button"
        >
          {t('serialLog.actions.clear')}
        </button>
      </div>

      <div
        aria-live="polite"
        className="theme-panel theme-border rounded-xl border p-4"
        ref={viewportRef}
        role="log"
      >
        {value.entries.length === 0 ? (
          <p className="theme-muted text-sm">{t('serialLog.empty')}</p>
        ) : (
          <ul className="flex flex-col gap-3">{renderedEntries}</ul>
        )}
      </div>

      <form className="space-y-3" onSubmit={handleSubmit}>
        <label className="flex flex-col gap-2 text-sm">
          <span className="font-medium">{t('serialLog.fields.payloadLabel')}</span>
          <textarea
            className="theme-panel theme-border min-h-[96px] rounded-xl border p-3 font-mono text-sm"
            onChange={handlePayloadChange}
            placeholder={t('serialLog.fields.payloadPlaceholder')}
            value={value.pendingPayload}
          />
        </label>
        <div className="flex justify-end">
          <button
            className="rounded-full bg-[rgb(var(--accent)/1)] px-4 py-2 text-sm font-medium text-[rgb(var(--surface-contrast)/1)] transition-transform hover:scale-[1.01]"
            type="submit"
            disabled={!value.pendingPayload.trim()}
          >
            {t('serialLog.actions.send')}
          </button>
        </div>
      </form>
    </div>
  );
};
