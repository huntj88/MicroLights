import { FingerChip } from '@/components/FingerChip';
import { FINGERS_BY_HAND, type Finger } from '@/lib/fingers';

export function FingerSelector({
  modeId,
  owner,
  onToggle,
}: {
  modeId: string;
  owner: Record<Finger, string | null>;
  onToggle: (finger: Finger, owned: boolean) => void;
}) {
  return (
    <div className="grid grid-cols-2 gap-4">
      <div>
        <div className="text-xs text-slate-400 mb-1">Left</div>
        <div className="flex flex-col gap-2">
          {FINGERS_BY_HAND.L.map(f => (
            <FingerChip
              key={f}
              finger={f}
              owned={owner[f] === modeId}
              onToggle={() => onToggle(f, owner[f] === modeId)}
            />
          ))}
        </div>
      </div>
      <div>
        <div className="text-xs text-slate-400 mb-1">Right</div>
        <div className="flex flex-col gap-2">
          {FINGERS_BY_HAND.R.map(f => (
            <FingerChip
              key={f}
              finger={f}
              owned={owner[f] === modeId}
              onToggle={() => onToggle(f, owner[f] === modeId)}
            />
          ))}
        </div>
      </div>
    </div>
  );
}
