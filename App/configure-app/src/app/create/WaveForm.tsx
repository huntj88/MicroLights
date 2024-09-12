import { useEffect, useRef } from "react";

export type LogicLevel = "low" | "high";

export type BulbConfig = {
  name: string;
  type: "bulb";
  totalTicks: number;
  changeAt: LogicLevelChange[];
};

export type LogicLevelChange = {
  tick: number;
  output: LogicLevel;
};

export const WaveForm: React.FC<{ bulbconfig: BulbConfig }> = ({
  bulbconfig,
}) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const scaleFactor = 20;
  const padding = 2;

  useEffect(() => {
    const ctx = canvasRef.current?.getContext("2d");
    if (!ctx) {
      return;
    }

    ctx.beginPath();
    ctx.strokeStyle = "green";
    ctx.lineWidth = 5;

    const pixelHeight = (change: LogicLevelChange) =>
      change.output === "high" ? 10 : 90;

    const startY = pixelHeight(bulbconfig.changeAt[0]);
    ctx.moveTo(padding, startY);

    let previousY = startY;
    bulbconfig.changeAt.forEach((change, index) => {
      const y = pixelHeight(change);
      if (index !== 0) {
        ctx.lineTo(padding + change.tick * scaleFactor, previousY);
      }
      ctx.lineTo(padding + change.tick * scaleFactor, y);
      previousY = y;
    });

    ctx.lineTo(padding + bulbconfig.totalTicks * scaleFactor, previousY);

    ctx.stroke();
  }, [bulbconfig]);

  return (
    <div>
      <canvas
        ref={(ref) => {
          canvasRef.current = ref;
        }}
        style={{ paddingTop: "16px" }}
        width={bulbconfig.totalTicks * scaleFactor + padding * 2}
        height={100}
      />
    </div>
  );
};
