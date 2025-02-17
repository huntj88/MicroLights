"use client";
import { useState } from "react";
import SetConfigCard, { ChipSetConfig } from "./SetConfigCard";

export default function Create() {
  const [chipSetConfig, setChipSetConfig] = useState<ChipSetConfig>({
    name: "test",
    parts: [
      {
        waveConfigName: undefined,
        fingers: [],
        caseColor: "#6699ff",
      },
      {
        waveConfigName: undefined,
        fingers: [],
        caseColor: "#000000",
      },
    ],
  });
  return (
    <div
      style={{
        display: "flex",
        width: "100%",
        padding: "16px",
      }}
    >
      <SetConfigCard
        chipSetConfig={chipSetConfig}
        partIndex={0}
        updateChipSetConfig={setChipSetConfig}
      />
      <SetConfigCard
        chipSetConfig={chipSetConfig}
        partIndex={1}
        updateChipSetConfig={setChipSetConfig}
      />
    </div>
  );
}
