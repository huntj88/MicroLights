"use client";
import {
  Card,
  makeStyles,
  OptionOnSelectData,
  SelectionEvents,
  Text,
  tokens,
} from "@fluentui/react-components";
import tinycolor, { HSVA, Numberify } from "@ctrl/tinycolor";
import { useCallback, useState } from "react";
import { WaveFormDropdown } from "@/components/wave/WaveFormDropdown";
import { FingerCheckboxArea } from "@/components/FingerCheckboxArea";
import { WaveForm, WaveFormConfig } from "@/components/wave/WaveForm";
import { ColorPicker } from "../ColorPicker";
import { cloneValue } from "@/utils";
import { waveFormPrefix } from "@/app/constants";

export type ChipSetConfig = {
  name: string;
  parts: ChipConfig[];
};

export type ChipConfig = {
  waveConfigName?: string;
  fingers: number[];
  caseColor: string;
};

interface SetConfigCardProps {
  chipSetConfig: ChipSetConfig;
  partIndex: number;
  updateChipSetConfig: (config: ChipSetConfig) => void;
}

const useStyles = makeStyles({
  previewColor: {
    width: "50px",
    height: "50px",
    borderRadius: tokens.borderRadiusMedium,
    border: `1px solid ${tokens.colorNeutralStroke1}`,
    margin: `${tokens.spacingVerticalMNudge} 0`,
  },
});

export default function SetConfigCard({
  chipSetConfig,
  partIndex,
  updateChipSetConfig,
}: SetConfigCardProps) {
  const styles = useStyles();

  const onColorChanged = useCallback(
    (color: Numberify<HSVA>) => {
      const clone = cloneValue(chipSetConfig);
      clone.parts[partIndex].caseColor = tinycolor(color).toHexString();
      updateChipSetConfig({ ...clone });
    },
    [chipSetConfig, partIndex, updateChipSetConfig],
  );

  const [config, setConfigState] = useState<WaveFormConfig | undefined>(
    undefined,
  );
  const onWaveFormSelected = useCallback(
    (_: SelectionEvents, data: OptionOnSelectData) => {
      if (data.optionText) {
        const jsonConfig = localStorage.getItem(
          `${waveFormPrefix}${data.optionText}`,
        );
        if (jsonConfig) {
          const newConfig = JSON.parse(jsonConfig);
          setConfigState(newConfig);
          const clone = cloneValue(chipSetConfig);
          clone.parts[partIndex].waveConfigName = data.optionText;
          updateChipSetConfig({ ...clone });
        }
      }
    },
    [chipSetConfig, partIndex, updateChipSetConfig],
  );

  const onSelectedFingersChange = useCallback(
    (selectedFingers: number[]) => {
      const clone = cloneValue(chipSetConfig);
      clone.parts[partIndex].fingers = selectedFingers;
      updateChipSetConfig({ ...clone });
    },
    [chipSetConfig, partIndex, updateChipSetConfig],
  );

  const selected = chipSetConfig.parts[partIndex].fingers;
  const disabled = chipSetConfig.parts
    .filter((_, index) => index !== partIndex)
    .flatMap((part) => part.fingers);

  return (
    <div
      style={{
        display: "flex",
        flexDirection: "column",
        width: "300px",
        padding: "16px",
      }}
    >
      <Card>
        <FingerCheckboxArea
          selectedFingers={selected}
          disabledFingers={disabled}
          onSelectedFingersChange={onSelectedFingersChange}
        />
        <WaveFormDropdown onOptionSelect={onWaveFormSelected} />
        {config && <WaveForm config={config} />}
        <Text>Case Light</Text>
        <div
          className={styles.previewColor}
          style={{
            backgroundColor: tinycolor(
              chipSetConfig.parts[partIndex].caseColor,
            ).toRgbString(),
          }}
        />
        <ColorPicker
          initialColor={chipSetConfig.parts[partIndex].caseColor}
          onColorChange={onColorChanged}
        />
      </Card>
    </div>
  );
}
