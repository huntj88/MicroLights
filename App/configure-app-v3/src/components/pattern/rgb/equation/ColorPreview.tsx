import { useEffect, useRef } from 'react';

import { useTheme } from '../../../../app/providers/theme-context';

interface ColorPreviewProps {
  redPoints: number[];
  greenPoints: number[];
  bluePoints: number[];
  currentTime: number; // in ms
  totalDuration: number; // in ms
}

export const ColorPreview = ({
  redPoints,
  greenPoints,
  bluePoints,
  currentTime,
  totalDuration,
}: ColorPreviewProps) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const { resolved: theme } = useTheme();

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const width = canvas.width;
    const height = canvas.height;

    // Get theme colors
    const style = getComputedStyle(canvas);
    const surfaceContrast = style.getPropertyValue('--surface-contrast').trim();

    ctx.clearRect(0, 0, width, height);

    // Draw the gradient strip
    const imageData = ctx.createImageData(width, height);
    const data = imageData.data;

    const totalPoints = Math.max(redPoints.length, greenPoints.length, bluePoints.length);

    if (totalPoints === 0) return;

    for (let x = 0; x < width; x++) {
      const pointIndex = Math.floor((x / width) * totalPoints);

      const r = redPoints[pointIndex] || 0;
      const g = greenPoints[pointIndex] || 0;
      const b = bluePoints[pointIndex] || 0;

      for (let y = 0; y < height; y++) {
        const index = (y * width + x) * 4;
        data[index] = r; // Red
        data[index + 1] = g; // Green
        data[index + 2] = b; // Blue
        data[index + 3] = 255; // Alpha
      }
    }

    ctx.putImageData(imageData, 0, 0);

    // Draw playhead
    const playheadX = (currentTime / totalDuration) * width;
    ctx.beginPath();
    ctx.moveTo(playheadX, 0);
    ctx.lineTo(playheadX, height);
    ctx.strokeStyle = `rgb(${surfaceContrast})`;
    ctx.lineWidth = 2;
    ctx.stroke();
  }, [redPoints, greenPoints, bluePoints, currentTime, totalDuration, theme]);

  // Calculate current color for a swatch
  const currentIndex = Math.floor((currentTime / totalDuration) * Math.max(redPoints.length, 1));
  const currentR = redPoints[currentIndex] || 0;
  const currentG = greenPoints[currentIndex] || 0;
  const currentB = bluePoints[currentIndex] || 0;

  return (
    <div className="flex flex-col gap-2">
      <div className="flex items-center gap-4">
        <div
          className="w-16 h-16 rounded border theme-border shadow-sm"
          style={{
            backgroundColor: `rgb(${currentR.toString()}, ${currentG.toString()}, ${currentB.toString()})`,
          }}
        />
        <div className="flex-1 h-16 bg-[rgb(var(--surface-raised))] rounded overflow-hidden relative border theme-border">
          <canvas ref={canvasRef} width={800} height={64} className="w-full h-full" />
        </div>
      </div>
    </div>
  );
};
