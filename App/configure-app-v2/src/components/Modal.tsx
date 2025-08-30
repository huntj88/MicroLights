import type { ReactNode } from 'react';
import { useTranslation } from 'react-i18next';

type ModalProps = {
  open: boolean;
  title?: string;
  onClose: () => void;
  children: ReactNode;
  footer?: ReactNode;
  size?: 'sm' | 'md' | 'lg';
};

export function Modal({ open, title, onClose, children, footer, size = 'md' }: ModalProps) {
  const { t } = useTranslation();
  if (!open) return null;
  const maxW = size === 'sm' ? 'max-w-md' : size === 'lg' ? 'max-w-4xl' : 'max-w-2xl';
  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center">
      <div className="absolute inset-0 bg-black/60" onClick={onClose} />
      <div
        className={`relative w-[95vw] ${maxW} bg-bg-card border border-slate-700/50 rounded-lg shadow-xl`}
      >
        <div className="flex items-center justify-between p-3 border-b border-slate-700/50">
          <div className="font-semibold">{title}</div>
          <button
            className="px-2 py-1 rounded hover:bg-slate-700/40 text-slate-300"
            aria-label={t('close')}
            onClick={onClose}
          >
            ×
          </button>
        </div>
        <div className="p-4">{children}</div>
        {footer && (
          <div className="p-3 border-t border-slate-700/50 flex justify-end gap-2">{footer}</div>
        )}
      </div>
    </div>
  );
}
