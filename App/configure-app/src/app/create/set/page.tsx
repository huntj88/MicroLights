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

const useStyles = makeStyles({
  previewColor: {
    width: "50px",
    height: "50px",
    borderRadius: tokens.borderRadiusMedium,
    border: `1px solid ${tokens.colorNeutralStroke1}`,
    margin: `${tokens.spacingVerticalMNudge} 0`,
  },
});

export default function Create() {
  const styles = useStyles();

  const defaultColor = "#8888ff";
  const defaultColorHSV = tinycolor(defaultColor).toHsv();
  const [color, setColor] = useState(defaultColorHSV);

  const onColorChanged = useCallback((color: Numberify<HSVA>) => {
    setColor(color);
  }, []);

  const [config, setConfig] = useState<WaveFormConfig | undefined>();
  const waveFormSelected = useCallback(
    (_: SelectionEvents, data: OptionOnSelectData) => {
      if (data.optionText) {
        const jsonConfig = localStorage.getItem(data.optionText);
        if (jsonConfig) {
          setConfig(JSON.parse(jsonConfig));
        }
      }
    },
    [],
  );

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
        <FingerCheckboxArea />
        <WaveFormDropdown onOptionSelect={waveFormSelected} />
        {config && <WaveForm config={config} />}
        <Text>Case Light</Text>
        <div
          className={styles.previewColor}
          style={{ backgroundColor: tinycolor(color).toRgbString() }}
        />
        <ColorPicker initialColor={defaultColor} onColorChange={onColorChanged} />
      </Card>
    </div>
  );
}
