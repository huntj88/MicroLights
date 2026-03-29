import { useTranslation } from 'react-i18next';

import { type BQ25180Registers, decodeBQ25180Registers } from '@/utils/bq25180-decoder';

import { Modal } from '../common/Modal';
import { StyledButton } from '../common/StyledButton';

interface ChargerStatusModalProps {
  isOpen: boolean;
  onClose: () => void;
  registers: BQ25180Registers;
}

export const ChargerStatusModal = ({ isOpen, onClose, registers }: ChargerStatusModalProps) => {
  const { t } = useTranslation();

  if (!isOpen) return null;

  const decodedRegisters = decodeBQ25180Registers(registers);

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={t('serialLog.chargerStatus.title')} maxWidth="lg">
      <header className="mb-4 flex items-center justify-between">
        <h3 className="text-xl font-semibold">{t('serialLog.chargerStatus.title')}</h3>
        <button
          onClick={onClose}
          className="rounded-lg p-2 hover:bg-[rgb(var(--surface-muted)/0.15)] transition-colors"
        >
          ✕
        </button>
      </header>

      <div className="flex-1 overflow-y-auto pr-2">
        <div className="space-y-6">
          {decodedRegisters.map(reg => (
            <div
              key={reg.name}
              className="rounded-lg border theme-border p-4"
            >
              <div className="mb-2 flex items-center justify-between">
                <h4 className="font-semibold theme-accent">{reg.name}</h4>
                <code className="rounded bg-[rgb(var(--surface)/0.5)] px-2 py-0.5 text-xs font-mono">
                  {reg.raw}
                </code>
              </div>

              {reg.fields.length > 0 ? (
                <div className="grid grid-cols-1 gap-2 sm:grid-cols-2">
                  {reg.fields.map(field => (
                    <div key={field.name} className="flex flex-col text-sm">
                      <span className="theme-muted">{field.name}</span>
                        <div className="flex items-center gap-2">
                        <span className="font-medium">{field.description}</span>
                        <span className="text-xs theme-muted">({field.value})</span>
                      </div>
                    </div>
                  ))}
                </div>
              ) : (
                <p className="text-sm italic theme-muted">
                  {t('serialLog.chargerStatus.noDecoding')}
                </p>
              )}
            </div>
          ))}
        </div>
      </div>

      <footer className="mt-6 flex justify-end gap-3 border-t theme-border pt-4">
        <StyledButton onClick={onClose}>{t('common.actions.close')}</StyledButton>
      </footer>
    </Modal>
  );
};
