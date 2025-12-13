import { useEffect, useRef } from 'react';
import { useTranslation } from 'react-i18next';

interface WaveformLaneProps {
  color: 'red' | 'green' | 'blue';
  points: number[];
  currentTime: number;
  totalDuration: number;
  height?: number;
}

export const WaveformLane = ({
  color,
  points,
  currentTime,
  totalDuration,
  height = 100,
}: WaveformLaneProps) => {
  const { t } = useTranslation();
  const canvasRef = useRef<HTMLCanvasElement>(null);

  const strokeColor = {
    red: '#ef4444',
    green: '#22c55e',
    blue: '#3b82f6',
  }[color];

  const fillColor = {
    red: 'rgba(239, 68, 68, 0.2)',
    green: 'rgba(34, 197, 94, 0.2)',
    blue: 'rgba(59, 130, 246, 0.2)',
  }[color];

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const width = canvas.width;
    const h = canvas.height;

    ctx.clearRect(0, 0, width, h);

    // Draw grid/axis
    ctx.strokeStyle = '#374151'; // gray-700
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, h);
    ctx.lineTo(width, h); // X axis
    ctx.stroke();

    if (points.length === 0) return;

    // Draw waveform
    ctx.beginPath();
    ctx.moveTo(0, h);

    for (let i = 0; i < points.length; i++) {
      const x = (i / (points.length - 1)) * width;
      const y = h - (points[i] / 255) * h;
      ctx.lineTo(x, y);
    }

    ctx.lineTo(width, h);
    ctx.closePath();

    ctx.fillStyle = fillColor;
    ctx.fill();

    ctx.strokeStyle = strokeColor;
    ctx.lineWidth = 2;
    ctx.stroke();

    // Draw playhead
    const playheadX = (currentTime / totalDuration) * width;
    ctx.beginPath();
    ctx.moveTo(playheadX, 0);
    ctx.lineTo(playheadX, h);
    ctx.strokeStyle = 'white';
    ctx.lineWidth = 1;
    ctx.setLineDash([4, 4]);
    ctx.stroke();
    ctx.setLineDash([]);
  }, [points, currentTime, totalDuration, strokeColor, fillColor]);

  return (
    <div className="relative bg-gray-900 rounded border border-gray-700 overflow-hidden">
      <div
        className="absolute top-1 left-2 text-xs font-bold uppercase opacity-50"
        style={{ color: strokeColor }}
      >
        {t(`rgbPattern.equation.colors.${color}`)}
      </div>
      <canvas ref={canvasRef} width={800} height={height} className="w-full h-full block" />
    </div>
  );
};
