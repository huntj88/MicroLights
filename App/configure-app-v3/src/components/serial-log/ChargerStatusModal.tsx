import { useTranslation } from 'react-i18next';

import {
  type BQ25180Registers,
  decodeBQ25180Registers,
} from '@/utils/bq25180-decoder';

import { StyledButton } from '../common/StyledButton';

interface ChargerStatusModalProps {
  isOpen: boolean;
  onClose: () => void;
  registers: BQ25180Registers;
}

export const ChargerStatusModal = ({
  isOpen,
  onClose,
  registers,
}: ChargerStatusModalProps) => {
  const { t } = useTranslation();

  if (!isOpen) return null;

  const decodedRegisters = decodeBQ25180Registers(registers);

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 p-4 backdrop-blur-sm">
      <div className="flex max-h-[90vh] w-full max-w-2xl flex-col rounded-xl bg-white p-6 shadow-xl dark:bg-gray-800">
        <header className="mb-4 flex items-center justify-between">
          <h3 className="text-xl font-semibold">{t('serialLog.chargerStatus.title')}</h3>
          <button
            onClick={onClose}
            className="rounded-lg p-2 hover:bg-gray-100 dark:hover:bg-gray-700"
          >
            ✕
          </button>
        </header>

        <div className="flex-1 overflow-y-auto pr-2">
          <div className="space-y-6">
            {decodedRegisters.map(reg => (
              <div key={reg.name} className="rounded-lg border border-gray-200 p-4 dark:border-gray-700">
                <div className="mb-2 flex items-center justify-between">
                  <h4 className="font-semibold text-blue-600 dark:text-blue-400">
                    {reg.name}
                  </h4>
                  <code className="rounded bg-gray-100 px-2 py-0.5 text-xs font-mono dark:bg-gray-900">
                    {reg.raw}
                  </code>
                </div>
                
                {reg.fields.length > 0 ? (
                  <div className="grid grid-cols-1 gap-2 sm:grid-cols-2">
                    {reg.fields.map(field => (
                      <div key={field.name} className="flex flex-col text-sm">
                        <span className="text-gray-500 dark:text-gray-400">{field.name}</span>
                        <div className="flex items-center gap-2">
                          <span className="font-medium">{field.description}</span>
                          <span className="text-xs text-gray-400">({field.value})</span>
                        </div>
                      </div>
                    ))}
                  </div>
                ) : (
                  <p className="text-sm italic text-gray-500">
                    {t('serialLog.chargerStatus.noDecoding')}
                  </p>
                )}
              </div>
            ))}
          </div>
        </div>

        <footer className="mt-6 flex justify-end gap-3 border-t border-gray-100 pt-4 dark:border-gray-700">
          <StyledButton onClick={onClose}>
            {t('common.actions.close')}
          </StyledButton>
        </footer>
      </div>
    </div>
  );
};
