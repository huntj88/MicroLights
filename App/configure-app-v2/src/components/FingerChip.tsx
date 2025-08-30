import { clsx } from 'clsx';

import type { Finger, Hand } from '@/lib/fingers';

export function FingerChip({
  finger,
  owned,
  onToggle,
}: {
  finger: Finger;
  owned: boolean;
  onToggle: () => void;
}) {
  const [hand, name] = finger.split('-') as [Hand, string];
  return (
    <button
      className={clsx(
        'px-2 py-1 rounded text-xs border border-slate-700/50',
        owned ? 'bg-fg-ring/80 text-slate-900' : 'bg-slate-800 text-slate-200 hover:bg-slate-700',
      )}
      onClick={onToggle}
    >
      <span className="opacity-70 mr-1">{hand === 'L' ? 'L' : 'R'}</span>
      {name}
    </button>
  );
}
