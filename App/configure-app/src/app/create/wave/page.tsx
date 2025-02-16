"use client";
import { FingerDropdown } from '@/app/FingerDropdown';
import { WaveFormEditWrapper } from '../WaveFormEditWrapper';

export default function Create() {
  return (
    <div style={{ display: "flex", flexDirection: "column", width: "100%" }}>
      <WaveFormEditWrapper name="hello world" />
    </div>
  );
}
