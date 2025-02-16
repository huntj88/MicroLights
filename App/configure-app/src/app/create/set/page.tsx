"use client";
import { Card, makeStyles, Text, tokens } from "@fluentui/react-components";
import { ColorPicker } from "../ColorPicker";
import tinycolor, { HSVA, Numberify } from "@ctrl/tinycolor";
import { useState } from "react";
import { FingerDropdown } from "@/app/FingerDropdown";
import { WaveFormDropdown } from "@/app/WaveFormDropdown";

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
  const DEFAULT_COLOR_HSV = tinycolor("#111111").toHsv();
  const [color, setColor] = useState(DEFAULT_COLOR_HSV);
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
        <FingerDropdown />
        <WaveFormDropdown />
        {/* <WaveForm bulbconfig={firstThenAllBulbConfig} /> */}
        <Text>Case Light</Text>
        <div
          className={styles.previewColor}
          style={{ backgroundColor: tinycolor(color).toRgbString() }}
        />
        <ColorPicker initialColor={"#8888ff"} onColorChange={handleChange} />
      </Card>
    </div>
  );
}
