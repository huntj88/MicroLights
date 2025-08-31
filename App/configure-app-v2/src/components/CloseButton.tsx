import { useTranslation } from 'react-i18next';

type Props = {
  onClick: () => void;
  className?: string;
};

export function CloseButton({ onClick, className }: Props) {
  const { t } = useTranslation();
  const base =
    'px-2 py-1 rounded border border-slate-600/60 bg-transparent hover:bg-slate-800 text-slate-200 text-xs';
  return (
    <button
      type="button"
      aria-label={t('remove')}
      title={t('remove')}
      className={`${base}${className ? ` ${className}` : ''}`}
      onClick={onClick}
    >
      x
    </button>
  );
}
