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
  const horizontalPadding = 16;
  const topPadding = 5;
  const indexCircleRadius = 10;

  useEffect(() => {
    const ctx = canvasRef.current?.getContext("2d");
    if (!ctx) {
      return;
    }

    ctx.beginPath();
    ctx.strokeStyle = "green";
    ctx.lineWidth = 7;

    const pixelHeight = (change: LogicLevelChange) =>
      (change.output === "high" ? 10 : 90) + topPadding;

    const startY = pixelHeight(bulbconfig.changeAt[0]);
    ctx.moveTo(horizontalPadding, startY);

    let previousY = startY;
    bulbconfig.changeAt.forEach((change, index) => {
      const x = horizontalPadding + change.tick * scaleFactor;
      const y = pixelHeight(change);
      if (index !== 0) {
        ctx.lineTo(x, previousY);
      }
      ctx.lineTo(x, y);
      previousY = y;
    });

    ctx.lineTo(
      horizontalPadding + bulbconfig.totalTicks * scaleFactor,
      previousY,
    );

    ctx.stroke();

    // index markers
    ctx.fillStyle = "black";
    ctx.strokeStyle = "white";
    ctx.lineWidth = 1;
    bulbconfig.changeAt.forEach((change, index) => {
      const x = horizontalPadding + change.tick * scaleFactor;
      const y = pixelHeight(change);
      ctx.beginPath();
      ctx.arc(x, y, indexCircleRadius, 0, 360);
      ctx.fill();
      ctx.stroke();
      ctx.strokeText(index + "", x - 3, y + 4);
      previousY = y;
    });

    return () => {
      ctx.reset();
    };
  }, [bulbconfig]);

  return (
    <div>
      <canvas
        ref={canvasRef}
        width={bulbconfig.totalTicks * scaleFactor + horizontalPadding * 2}
        height={90 + indexCircleRadius * 2}
      />
    </div>
  );
};
