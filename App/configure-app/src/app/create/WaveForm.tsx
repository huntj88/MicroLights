import { MouseEvent, useCallback, useEffect, useRef, useState } from "react";

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

export const WaveForm: React.FC<{
  bulbconfig: BulbConfig;
  updateConfig: (BulbConfig: BulbConfig) => void;
}> = ({ bulbconfig, updateConfig }) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const scaleFactor = 20;
  const horizontalPadding = 16;
  const indexCircleRadius = 10;
  const topPadding = 5;
  const pixelHeight = (change: LogicLevelChange) =>
    (change.output === "high" ? 10 : 90) + topPadding;

  const [mousePressed, setMousePressed] = useState(false);
  const [mousePosition, setMousePosition] = useState<{ x: number; y: number }>({
    x: 0,
    y: 0,
  });

  const [selectedIndex, setSelectedIndex] = useState<number | undefined>();

  useEffect(() => {
    if (!mousePressed) {
      setSelectedIndex(undefined);
      return;
    }

    if (selectedIndex) {
      return;
    }

    bulbconfig.changeAt.forEach((change, index) => {
      const x = horizontalPadding + change.tick * scaleFactor;
      const y = pixelHeight(change);

      if (
        Math.abs(mousePosition.x - x) < 10 &&
        Math.abs(mousePosition.y - y) < 10 &&
        !selectedIndex
      ) {
        setSelectedIndex(index);
      }
    });
  }, [
    mousePressed,
    mousePosition.x,
    mousePosition.y,
    bulbconfig,
    selectedIndex,
  ]);

  useEffect(() => {
    if (selectedIndex) {
      const copy = { ...bulbconfig };
      const current = copy.changeAt[selectedIndex];
      const x = horizontalPadding + current.tick * scaleFactor;

      const tickOffset =
        mousePosition.x - x > 20 ? 1 : mousePosition.x - x < -20 ? -1 : 0;
      const newTickLocation = current.tick + tickOffset;

      copy.changeAt[selectedIndex] = {
        tick: newTickLocation,
        output: current.output,
      };
      updateConfig(copy);
    }
  }, [
    // bulbConfig intentionally excluded. TODO: lint rule
    mousePosition,
    selectedIndex,
    updateConfig,
  ]);

  useEffect(() => {
    const ctx = canvasRef.current?.getContext("2d");
    if (!ctx) {
      return;
    }

    ctx.beginPath();
    ctx.strokeStyle = "green";
    ctx.lineWidth = 7;

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

  const onMouseEnter = useCallback((e: MouseEvent<HTMLCanvasElement>) => {
    if (e.buttons === 0) {
      setMousePressed(false);
    }
  }, []);

  const onMouseDown = useCallback(() => {
    setMousePressed(true);
  }, []);

  const onMouseUp = useCallback(() => {
    setMousePressed(false);
  }, []);

  const onMouseMove = useCallback((e: MouseEvent<HTMLCanvasElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    setMousePosition({ x, y });
  }, []);

  return (
    <div>
      <canvas
        ref={canvasRef}
        width={bulbconfig.totalTicks * scaleFactor + horizontalPadding * 2}
        height={90 + indexCircleRadius * 2}
        onMouseEnter={onMouseEnter}
        onMouseDown={onMouseDown}
        onMouseUp={onMouseUp}
        onMouseMove={onMouseMove}
      />
    </div>
  );
};
