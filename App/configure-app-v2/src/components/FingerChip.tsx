import { clsx } from 'clsx';
import { useTranslation } from 'react-i18next';

import type { Finger } from '@/lib/fingers';

export function FingerChip({
  finger,
  owned,
  onToggle,
}: {
  finger: Finger;
  owned: boolean;
  onToggle: () => void;
}) {
  const { t } = useTranslation();
  return (
    <button
      className={clsx(
        'px-2 py-1 rounded text-xs border border-slate-700/50',
        owned ? 'bg-fg-ring/80 text-slate-900' : 'bg-slate-800 text-slate-200 hover:bg-slate-700',
      )}
      onClick={onToggle}
    >
      {t(`fingerLabel.${finger}` as const)}
    </button>
  );
}
