import { Button } from "@fluentui/react-components";
import {
  MouseEvent,
  TouchEvent,
  useCallback,
  useEffect,
  useRef,
  useState,
} from "react";

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

  const [mousePressed, setMousePressed] = useState(false);
  const [mousePosition, setMousePosition] = useState<{ x: number; y: number }>({
    x: 0,
    y: 0,
  });

  const [selectedIndex, setSelectedIndex] = useState<number | undefined>();

  const pixelHeight = useCallback(
    (change: LogicLevelChange, index?: number) => {
      if (index !== undefined && selectedIndex === index && mousePressed) {
        return mousePosition.y;
      }
      return (change?.output === "high" ? 10 : 90) + topPadding;
    },
    [selectedIndex, mousePressed, mousePosition],
  );

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

    if (selectedIndex !== undefined) {
      return;
    }

    bulbconfig.changeAt.forEach((change, index) => {
      const x =
        horizontalPadding + change.tick * scaleFactor + indexCircleRadius;
      const y = pixelHeight(change, undefined) + indexCircleRadius;

      if (
        Math.abs(mousePosition.x - x) < indexCircleRadius * 1.25 &&
        Math.abs(mousePosition.y - y) < indexCircleRadius * 1.25 &&
        selectedIndex === undefined
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
    pixelHeight,
    scaleFactor,
    selectedIndex,
  ]);

  useEffect(() => {
    if (selectedIndex !== undefined) {
      const copy = { ...bulbconfig };
      const current = copy.changeAt[selectedIndex];
      const x = horizontalPadding + current.tick * scaleFactor;

      const tickOffset =
        mousePosition.x - x > 20 ? 1 : mousePosition.x - x < -20 ? -1 : 0;
      const newTickLocation = current.tick + tickOffset;

      const verticalPosition: LogicLevel =
        mousePosition.y > 50 ? "low" : "high";

      const change = {
        tick: newTickLocation,
        output: verticalPosition,
      };
      copy.changeAt[selectedIndex] = change;
      copy.changeAt.sort((a, b) => a.tick - b.tick);

      setSelectedIndex(copy.changeAt.indexOf(change));

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

    let previousX = horizontalPadding;
    let previousY = pixelHeight(bulbconfig.changeAt[0], undefined);
    ctx.lineWidth = 7;
    ctx.strokeStyle = bulbconfig.changeAt[0].output == "high" ? "green" : "red"

    ctx.beginPath();
    ctx.moveTo(previousX, previousY);
    // render first horizontal bar before first config
    ctx.lineTo(horizontalPadding + bulbconfig.changeAt[0].tick * scaleFactor, previousY)

    bulbconfig.changeAt.forEach((change) => {
      const x = horizontalPadding + change.tick * scaleFactor;
      const y = pixelHeight(change, undefined);

      const renderOffsetX = change.output == "high" ? - 3 : 3;
      const renderOffsetY = change.output == "high" ? 3 : -3;
      
      // horizontal bar
      ctx.lineTo(x + renderOffsetX, previousY)

      if (previousY !== y) {
        ctx.stroke()
        ctx.beginPath();
        ctx.strokeStyle = change.output == "high" ? "green" : "red";
        ctx.moveTo(x, previousY + renderOffsetY)
      }

      // vertical bar
      ctx.lineTo(x, y)

      previousX = x;
      previousY = y;
    })

    // render last horizontal bar after last config
    ctx.lineTo(horizontalPadding + bulbconfig.totalTicks * scaleFactor, previousY)
    ctx.stroke()

    // index markers
    const previewMarker = () => {
      if (selectedIndex === undefined) return;
      ctx.fillStyle = "#222222";
      ctx.strokeStyle = "#777777";

      const change = bulbconfig.changeAt[selectedIndex];
      const x = horizontalPadding + change.tick * scaleFactor;
      const y = pixelHeight(change, undefined);
      renderMarker(x, y, selectedIndex);
    };

    const tickMarker = (change: LogicLevelChange, index: number) => {
      ctx.fillStyle = "black";
      ctx.strokeStyle = "white";
      const x =
        index === selectedIndex
          ? mousePosition.x
          : horizontalPadding + change.tick * scaleFactor;
      const y = pixelHeight(change, index);
      renderMarker(x, y, index);
    };

    const renderMarker = (x: number, y: number, index: number) => {
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.arc(x, y, indexCircleRadius, 0, 360);
      ctx.fill();
      ctx.stroke();
      const text = index + "";
      const metrics = ctx.measureText(text);
      ctx.strokeText(
        text,
        x - metrics.width / 2,
        y + metrics.hangingBaseline / 2,
      );
    };

    previewMarker();
    bulbconfig.changeAt.forEach((change, index) => {
      tickMarker(change, index);
      previousY = pixelHeight(change, index);
    });

    return () => {
      ctx.reset();
    };
  }, [
    bulbconfig,
    canvasRef.current?.offsetHeight,
    canvasRef.current?.offsetWidth,
    horizontalPadding,
    mousePosition.x,
    scaleFactor,
    selectedIndex,
    pixelHeight,
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

  const onTouchMove = useCallback((e: TouchEvent<HTMLCanvasElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.touches.item(0).clientX - rect.left;
    const y = e.touches.item(0).clientY - rect.top;
    setMousePosition({ x, y });
  }, []);

  const onAddChange = useCallback(() => {
    const copy = { ...bulbconfig };
    const lastIndex = copy.changeAt.length - 1;
    const last = copy.changeAt[lastIndex];
    const newChange: LogicLevelChange = {
      tick: last ? last.tick + 1 : 0,
      output: "high",
    };
    copy.changeAt = copy.changeAt.concat([newChange]);
    copy.totalTicks = Math.max(copy.totalTicks, newChange.tick);
    updateConfig(copy);
  }, [bulbconfig, updateConfig]);

  const onRemoveChange = useCallback(() => {
    const copy = { ...bulbconfig };
    copy.changeAt = copy.changeAt.slice(0, -1);
    updateConfig(copy);
  }, [bulbconfig, updateConfig]);

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
        onTouchStart={onMouseDown}
        onTouchEnd={onMouseUp}
        onTouchMove={onTouchMove}
      />
      <Button onClick={onAddChange}>Add</Button>
      <Button onClick={onRemoveChange}>Remove</Button>
    </div>
  );
};
