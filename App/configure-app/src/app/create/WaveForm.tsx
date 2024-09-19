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
  const [canvas, setCanvas] = useState<HTMLCanvasElement | null>(
    canvasRef.current,
  );
  const scaleFactor = (canvasRef.current?.width ?? 0) / 71.5;
  const horizontalPadding = (canvasRef.current?.width ?? 0) / 100;
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
    if (canvasRef.current) {
      canvasRef.current.width = canvasRef.current?.offsetWidth;
      canvasRef.current.height = canvasRef.current?.offsetHeight;
    }
  }, [canvas, canvasRef.current?.offsetHeight, canvasRef.current?.offsetWidth]);

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
    bulbconfig,
    horizontalPadding,
    mousePressed,
    mousePosition.x,
    mousePosition.y,
    scaleFactor,
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
    horizontalPadding,
    mousePosition,
    scaleFactor,
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
  }, [
    bulbconfig,
    canvasRef.current?.offsetHeight,
    canvasRef.current?.offsetWidth,
    horizontalPadding,
    scaleFactor,
  ]);

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
        ref={(ref: HTMLCanvasElement) => {
          canvasRef.current = ref;
          setCanvas(ref);
        }}
        style={{ width: "100%", height: "100%" }}
        onMouseEnter={onMouseEnter}
        onMouseDown={onMouseDown}
        onMouseUp={onMouseUp}
        onMouseMove={onMouseMove}
      />
    </div>
  );
};
