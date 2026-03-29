import { FocusTrap } from 'focus-trap-react';
import { type ReactNode, useEffect, useRef } from 'react';

const maxWidthMap = {
  sm: 'sm:max-w-sm',
  md: 'sm:max-w-md',
  lg: 'sm:max-w-2xl',
} as const;

interface ModalProps {
  isOpen: boolean;
  onClose: () => void;
  title?: string;
  children: ReactNode;
  maxWidth?: 'sm' | 'md' | 'lg';
}

export const Modal = ({ isOpen, onClose, title, children, maxWidth = 'md' }: ModalProps) => {
  const dialogRef = useRef<HTMLDivElement>(null);

  // Close on Escape key
  useEffect(() => {
    if (!isOpen) return;

    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        onClose();
      }
    };
    document.addEventListener('keydown', handleKeyDown);
    return () => {
      document.removeEventListener('keydown', handleKeyDown);
    };
  }, [isOpen, onClose]);

  // Lock body scroll when open
  useEffect(() => {
    if (!isOpen) return;
    const prev = document.body.style.overflow;
    document.body.style.overflow = 'hidden';
    return () => {
      document.body.style.overflow = prev;
    };
  }, [isOpen]);

  if (!isOpen) return null;

  const handleBackdropClick = (e: React.MouseEvent) => {
    // Only close if the click is directly on the backdrop (not on the dialog)
    if (e.target === e.currentTarget) {
      onClose();
    }
  };

  return (
    <FocusTrap
      focusTrapOptions={{
        allowOutsideClick: true,
        escapeDeactivates: false, // We handle Escape ourselves
        fallbackFocus: () => dialogRef.current ?? document.body,
      }}
    >
      <div
        className="fixed inset-0 z-50 flex items-end sm:items-center justify-center bg-black/50 backdrop-blur-sm transition-opacity"
        onClick={handleBackdropClick}
        role="presentation"
      >
        <div
          ref={dialogRef}
          aria-modal="true"
          role="dialog"
          aria-label={title}
          className={`w-full ${maxWidthMap[maxWidth]} max-h-[90vh] overflow-y-auto rounded-t-2xl sm:rounded-xl p-4 sm:p-6 bg-[rgb(var(--surface-raised)/1)] border theme-border shadow-xl transform transition-transform translate-y-0`}
          tabIndex={-1}
        >
          {children}
        </div>
      </div>
    </FocusTrap>
  );
};
