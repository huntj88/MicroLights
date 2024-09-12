"use client";
import { Text, Textarea } from "@fluentui/react-components";
import { WaveForm } from "./WaveForm";
import {
  firstThenAllBulbConfig,
  flashyBulbConfig,
  firstColorBulbConfig,
} from "../config";

export default function Create() {
  return (
    <div style={{ display: "flex", flexDirection: "column" }}>
      <Text>Json Config</Text>
      <Textarea
        style={{ width: 200, height: 250 }}
        value={JSON.stringify(firstThenAllBulbConfig, null, 2)}
      />
      <WaveForm bulbconfig={firstThenAllBulbConfig} />
      {/* <Textarea style={{width: 200, height: 250}} value={JSON.stringify(flashyBulbConfig, null, 2)} /> */}
      <WaveForm bulbconfig={flashyBulbConfig} />
      {/* <Textarea style={{width: 200, height: 250}} value={JSON.stringify(firstColorBulbConfig, null, 2)} /> */}
      <WaveForm bulbconfig={firstColorBulbConfig} />
    </div>
  );
}
