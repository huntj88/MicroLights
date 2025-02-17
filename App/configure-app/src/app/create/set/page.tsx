"use client";
import { Card, makeStyles, Text, tokens } from "@fluentui/react-components";
import tinycolor, { HSVA, Numberify } from "@ctrl/tinycolor";
import { useState } from "react";
import { WaveFormDropdown } from "@/components/wave/WaveFormDropdown";
import { firstThenAllBulbConfig } from "@/app/config";
import { FingerCheckboxArea } from "@/components/FingerCheckboxArea";
import { WaveForm } from "@/components/wave/WaveForm";
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
  const handleChange = (color: Numberify<HSVA>) => {
    setColor(color);
  };

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
        <WaveFormDropdown />
        <FingerCheckboxArea />
        <WaveForm config={firstThenAllBulbConfig} />
        <Text>Case Light</Text>
        <div
          className={styles.previewColor}
          style={{ backgroundColor: tinycolor(color).toRgbString() }}
        />
        <ColorPicker initialColor={defaultColor} onColorChange={handleChange} />
      </Card>
    </div>
  );
}
