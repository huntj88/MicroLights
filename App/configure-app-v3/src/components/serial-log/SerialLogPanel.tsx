import type { ChangeEvent, FormEvent } from 'react';
import { useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { type Mode } from '@/app/models/mode';
import { type BQ25180Registers } from '@/utils/bq25180-decoder';

import { ChargerStatusModal } from './ChargerStatusModal';
import { ImportModeModal } from './ImportModeModal';
import { StyledButton } from '../common/StyledButton';

export type SerialLogDirection = 'inbound' | 'outbound';

export interface SerialLogEntry {
  id: string;
  timestamp: string;
  direction: SerialLogDirection;
  payload: string;
}

export interface SerialLogState {
  entries: SerialLogEntry[];
  pendingPayload: string;
}

export type SerialLogAction =
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
  const [selectedChargerRegisters, setSelectedChargerRegisters] = useState<BQ25180Registers | null>(
    null,
  );
  const [selectedMode, setSelectedMode] = useState<Mode | null>(null);

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
        entries: [entry, ...value.entries],
        pendingPayload: '',
      },
      { type: 'submit', entry },
    );
  };

  const renderedEntries = useMemo(
    () =>
      value.entries.map(entry => {
        let actionButton = null;
        try {
          const data = JSON.parse(entry.payload) as unknown;

          if (typeof data === 'object' && data !== null) {
            const obj = data as Record<string, unknown>;
            // Check for charger registers
            // We check for a few key registers to identify it as charger data
            if ('chargectrl0' in obj || 'stat0' in obj || 'ichg_ctrl' in obj) {
              actionButton = (
                <StyledButton
                  onClick={() => {
                    setSelectedChargerRegisters(obj as unknown as BQ25180Registers);
                  }}
                  variant="secondary"
                  className="mt-2 text-xs"
                >
                  {t('serialLog.actions.viewChargerStatus')}
                </StyledButton>
              );
            }
            // Check for mode data
            else if ('mode' in obj && typeof obj.mode === 'object' && obj.mode !== null) {
              actionButton = (
                <StyledButton
                  onClick={() => {
                    setSelectedMode(obj.mode as Mode);
                  }}
                  variant="secondary"
                  className="mt-2 text-xs"
                >
                  {t('serialLog.actions.importMode')}
                </StyledButton>
              );
            }
          }
        } catch {
          // Ignore parse errors
        }

        return (
          <li
            key={entry.id}
            className="flex flex-col gap-1 border-b border-dashed theme-border pb-2 last:border-none last:pb-0"
          >
            <div className="flex items-start justify-between gap-2">
              <div className="flex flex-col gap-1 flex-1 min-w-0">
                <span className="font-mono text-xs theme-muted">
                  {timestampFormatter.format(new Date(entry.timestamp))} ·{' '}
                  {t(`serialLog.direction.${entry.direction}`)}
                </span>
                <code className="whitespace-pre-wrap break-words text-sm">{entry.payload}</code>
              </div>
              {actionButton && <div className="shrink-0">{actionButton}</div>}
            </div>
          </li>
        );
      }),
    [t, value.entries],
  );

  return (
    <div className="space-y-6">
      <div className="flex flex-wrap items-center gap-2">
        <button
          className="rounded-full border border-solid theme-border px-3 py-1 text-sm transition-colors hover:bg-[rgb(var(--surface-raised)/1)]"
          disabled={value.entries.length === 0}
          onClick={handleClear}
          type="button"
        >
          {t('serialLog.actions.clear')}
        </button>
      </div>

      <div aria-live="polite" className="theme-panel theme-border rounded-xl border p-4" role="log">
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

      {selectedChargerRegisters && (
        <ChargerStatusModal
          isOpen={!!selectedChargerRegisters}
          onClose={() => {
            setSelectedChargerRegisters(null);
          }}
          registers={selectedChargerRegisters}
        />
      )}

      {selectedMode && (
        <ImportModeModal
          isOpen={!!selectedMode}
          onClose={() => {
            setSelectedMode(null);
          }}
          mode={selectedMode}
        />
      )}
    </div>
  );
};
