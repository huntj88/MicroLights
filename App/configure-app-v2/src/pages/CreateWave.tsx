import { useState } from 'react';

import { WaveformEditor } from '@/components/WaveformEditor';
import type { Waveform } from '@/lib/waveform';

const initial: Waveform = {
  name: 'example',
  totalTicks: 33,
  changeAt: [
    { tick: 0, output: 'high' },
    { tick: 11, output: 'low' },
    { tick: 12, output: 'high' },
    { tick: 22, output: 'low' },
  ],
};

export default function CreateWave() {
  const [wf, setWf] = useState<Waveform>(initial);
  return (
    <div className="space-y-6">
      <h1 className="text-2xl font-semibold">Create / Wave</h1>
      <WaveformEditor value={wf} onChange={setWf} />
    </div>
  );
}
