import {
  MouseEvent,
  TouchEvent,
  useCallback,
  useEffect,
  useRef,
  useState,
} from "react";

export type LogicLevel = "low" | "high";

export type WaveFormConfig = {
  name: string;
  totalTicks: number;
  changeAt: LogicLevelChange[];
};

export type LogicLevelChange = {
  tick: number;
  output: LogicLevel;
};

export interface WaveFormProps {
  config: WaveFormConfig;
  updateConfig?: (config: WaveFormConfig) => void;
}

export const WaveForm: React.FC<WaveFormProps> = (props: {
  config: WaveFormConfig;
  updateConfig?: (config: WaveFormConfig) => void;
}) => {
  const { config, updateConfig } = props;

  const canEdit = updateConfig !== undefined;
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [canvas, setCanvas] = useState<HTMLCanvasElement | null>(
    canvasRef.current,
  );

  const height = 200;
  const scaleFactor = (canvasRef.current?.width ?? 0) / 71.5;
  const horizontalPadding = (canvasRef.current?.width ?? 0) / 100;
  const indexCircleRadius = 17;
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
      return (
        (change?.output === "high"
          ? indexCircleRadius
          : height - indexCircleRadius) + topPadding
      );
    },
    [selectedIndex, mousePressed, mousePosition],
  );

  useEffect(() => {
    if (canvasRef.current) {
      canvasRef.current.width = canvasRef.current?.offsetWidth;
      canvasRef.current.height = canvasRef.current?.offsetHeight;
    }
  }, [canvas, canvasRef.current?.offsetHeight, canvasRef.current?.offsetWidth]);

  // handles selecting new marker
  useEffect(() => {
    if (!canEdit) {
      return;
    }
    if (!mousePressed) {
      setSelectedIndex(undefined);
      return;
    }

    if (selectedIndex !== undefined) {
      return;
    }

    config.changeAt.forEach((change, index) => {
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
    canEdit,
    config,
    horizontalPadding,
    mousePressed,
    mousePosition.x,
    mousePosition.y,
    pixelHeight,
    scaleFactor,
    selectedIndex,
  ]);

  // handles moving the selected index around
  useEffect(() => {
    if (selectedIndex !== undefined && canEdit) {
      const copy = { ...config };
      const current = copy.changeAt[selectedIndex];
      const x = horizontalPadding + current.tick * scaleFactor;

      const tickOffset =
        mousePosition.x - x > 20 ? 1 : mousePosition.x - x < -20 ? -1 : 0;
      const newTickLocation = current.tick + tickOffset;

      const verticalPosition: LogicLevel =
        mousePosition.y > height / 2 ? "low" : "high";

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
    canEdit,
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

    const renderWaveFormLines = () => {
      const defaultIfEmpty = { output: "high", tick: 0 };
      const firstChange = config.changeAt[0] ?? defaultIfEmpty;

      let previousX = horizontalPadding;
      let previousY = pixelHeight(firstChange, undefined);

      const colorOn = "green";
      const colorOff = "#CC0000";
      ctx.strokeStyle = firstChange.output == "high" ? colorOn : colorOff;
      ctx.lineWidth = 7;

      ctx.beginPath();
      ctx.moveTo(previousX, previousY);
      // render first horizontal bar before first config
      ctx.lineTo(horizontalPadding + firstChange.tick * scaleFactor, previousY);

      config.changeAt.forEach((change) => {
        const x = horizontalPadding + change.tick * scaleFactor;
        const y = pixelHeight(change, undefined);

        const renderOffsetX = change.output == "high" ? -3 : 3;
        const renderOffsetY = change.output == "high" ? 3 : -3;

        // horizontal bar
        ctx.lineTo(x + renderOffsetX, previousY);

        if (previousY !== y) {
          ctx.stroke();
          ctx.beginPath();
          ctx.strokeStyle = change.output == "high" ? colorOn : colorOff;
          ctx.moveTo(x, previousY + renderOffsetY);
        }

        // vertical bar
        ctx.lineTo(x, y);

        previousX = x;
        previousY = y;
      });

      // render last horizontal bar after last config
      ctx.lineTo(
        horizontalPadding + config.totalTicks * scaleFactor,
        previousY,
      );
      ctx.stroke();
    };

    // index markers
    const renderPreviewMarker = () => {
      if (selectedIndex === undefined) return;
      ctx.fillStyle = "#222222aa";
      ctx.strokeStyle = "#777777";

      const change = config.changeAt[selectedIndex];
      const x = horizontalPadding + change.tick * scaleFactor;
      const y = pixelHeight(change, undefined);
      renderMarker(x, y, selectedIndex);
    };

    const renderTickMarker = (change: LogicLevelChange, index: number) => {
      ctx.fillStyle = "#000000aa";
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

    renderWaveFormLines();
    renderPreviewMarker();
    config.changeAt.forEach((change, index) => {
      renderTickMarker(change, index);
    });

    return () => {
      ctx.reset();
    };
  }, [
    config,
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

  const onMouseMove = useCallback((e: MouseEvent<HTMLCanvasElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    setMousePosition({ x, y });
  }, []);

  const onMouseDown = useCallback(
    (e: MouseEvent<HTMLCanvasElement>) => {
      setMousePressed(true);
      onMouseMove(e);
    },
    [onMouseMove],
  );

  const onMouseUp = useCallback(() => {
    setMousePressed(false);
  }, []);

  const onTouchMove = useCallback((e: TouchEvent<HTMLCanvasElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.touches.item(0).clientX - rect.left;
    const y = e.touches.item(0).clientY - rect.top;
    setMousePosition({ x, y });
  }, []);

  const onTouchDown = useCallback(
    (e: TouchEvent<HTMLCanvasElement>) => {
      setMousePressed(true);
      onTouchMove(e);
    },
    [onTouchMove],
  );

  return (
    <div>
      <canvas
        ref={(ref: HTMLCanvasElement) => {
          canvasRef.current = ref;
          setCanvas(ref);
        }}
        style={{ width: "100%", height: height + topPadding + 1 }}
        onMouseEnter={onMouseEnter}
        onMouseDown={onMouseDown}
        onMouseUp={onMouseUp}
        onMouseMove={onMouseMove}
        onTouchStart={onTouchDown}
        onTouchEnd={onMouseUp}
        onTouchMove={onTouchMove}
      />
    </div>
  );
};
