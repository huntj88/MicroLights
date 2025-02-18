"use client";

import { WaveFormEditWrapper } from "@/components/wave/WaveFormEditWrapper";

export default function Create() {
  return (
    <div style={{ display: "flex", flexDirection: "column", width: "100%" }}>
      <WaveFormEditWrapper name={""} />
    </div>
  );
}
