"use client";
import { Text, Textarea } from "@fluentui/react-components";
import { BulbConfig, WaveForm } from "./WaveForm";

const firstColorBulbConfig: BulbConfig = {
  name: "first",
  type: "bulb",
  totalTicks: 5,
  changeAt: [
    {
      tick: 0,
      output: "high",
    },
    {
      tick: 4,
      output: "low",
    },
  ],
};

const flashyBulbConfig: BulbConfig = {
  name: "flashy",
  type: "bulb",
  totalTicks: 30,
  changeAt: [
    {
      tick: 0,
      output: "high",
    },
    {
      tick: 8,
      output: "low",
    },
  ],
};

const firstThenAllBulbConfig: BulbConfig = {
  name: "default",
  type: "bulb",
  totalTicks: 70,
  changeAt: [
    {
      tick: 0,
      output: "high",
    },
    {
      tick: 6,
      output: "low",
    },
    {
      tick: 7,
      output: "high",
    },
    {
      tick: 14,
      output: "low",
    },
    {
      tick: 15,
      output: "high",
    },
    {
      tick: 22,
      output: "low",
    },
    {
      tick: 23,
      output: "high",
    },
    {
      tick: 30,
      output: "low",
    },
    {
      tick: 31,
      output: "high",
    },
    {
      tick: 70,
      output: "low",
    },
  ],
};

export default function Home() {
  return (
    <div style={{ display: "flex", flexDirection: "column" }}>
      <Text>Json Config</Text>
      <Textarea style={{width: 200, height: 250}} value={JSON.stringify(firstThenAllBulbConfig, null, 2)} />
      <WaveForm bulbconfig={firstThenAllBulbConfig} />
      {/* <Textarea style={{width: 200, height: 250}} value={JSON.stringify(flashyBulbConfig, null, 2)} /> */}
      <WaveForm bulbconfig={flashyBulbConfig} />
      {/* <Textarea style={{width: 200, height: 250}} value={JSON.stringify(firstColorBulbConfig, null, 2)} /> */}
      <WaveForm bulbconfig={firstColorBulbConfig} />
    </div>
  );
}
