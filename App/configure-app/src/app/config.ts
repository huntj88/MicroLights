import { WaveFormConfig } from "./create/WaveForm";

export const firstColorBulbConfig: WaveFormConfig = {
  name: "first",
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

export const flashyBulbConfig: WaveFormConfig = {
  name: "flashy",
  totalTicks: 30,
  changeAt: [
    {
      tick: 0,
      output: "high",
    },
    {
      tick: 12,
      output: "low",
    },
  ],
};

export const firstThenAllBulbConfig: WaveFormConfig = {
  name: "default",
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
