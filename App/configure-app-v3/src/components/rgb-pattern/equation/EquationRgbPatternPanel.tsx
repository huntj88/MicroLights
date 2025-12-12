import { useEffect, useMemo, useRef, useState } from 'react';
import {
  createDefaultEquationPattern,
  type EquationPattern,
  type EquationSection,
} from '../../../app/models/equation-pattern';
import { generateWaveformPoints } from '../../../utils/equation-evaluator';
import { ColorPreview } from './ColorPreview';
import { SectionLane } from './SectionLane';
import { WaveformLane } from './WaveformLane';

export const EquationRgbPatternPanel = () => {
  const [pattern, setPattern] = useState<EquationPattern>(createDefaultEquationPattern());
  const [isPlaying, setIsPlaying] = useState(false);
  const [currentTime, setCurrentTime] = useState(0);
  const requestRef = useRef<number>(0);
  const lastTimeRef = useRef<number>(0);

  const calculateChannelDuration = (sections: EquationSection[]) =>
    sections.reduce((acc, section) => acc + section.duration, 0);

  const redDuration = useMemo(() => calculateChannelDuration(pattern.red.sections), [pattern.red.sections]);
  const greenDuration = useMemo(() => calculateChannelDuration(pattern.green.sections), [pattern.green.sections]);
  const blueDuration = useMemo(() => calculateChannelDuration(pattern.blue.sections), [pattern.blue.sections]);

  const totalDuration = Math.max(redDuration, greenDuration, blueDuration, 1000); // Min 1s for display

  // Generate waveforms
  // We generate points for the entire duration.
  // Sample rate 10ms is fine for display.
  const redPoints = useMemo(
    () => generateWaveformPoints(pattern.red.sections, totalDuration),
    [pattern.red.sections, totalDuration]
  );
  const greenPoints = useMemo(
    () => generateWaveformPoints(pattern.green.sections, totalDuration),
    [pattern.green.sections, totalDuration]
  );
  const bluePoints = useMemo(
    () => generateWaveformPoints(pattern.blue.sections, totalDuration),
    [pattern.blue.sections, totalDuration]
  );

  const animate = (time: number) => {
    if (lastTimeRef.current === 0) {
      lastTimeRef.current = time;
    }
    
    const deltaTime = time - lastTimeRef.current;
    lastTimeRef.current = time;

    setCurrentTime(prev => {
      const next = prev + deltaTime;
      if (next >= totalDuration) {
        // Loop or stop? Let's loop for now or stop.
        // User usually wants loop for patterns.
        return 0; 
      }
      return next;
    });

    requestRef.current = requestAnimationFrame(animate);
  };

  useEffect(() => {
    if (isPlaying) {
      lastTimeRef.current = 0;
      requestRef.current = requestAnimationFrame(animate);
    } else {
      if (requestRef.current) {
        cancelAnimationFrame(requestRef.current);
      }
    }
    return () => {
      if (requestRef.current) {
        cancelAnimationFrame(requestRef.current);
      }
    };
  }, [isPlaying, totalDuration]);

  const handlePlayPause = () => {
    setIsPlaying(!isPlaying);
  };

  const handleStop = () => {
    setIsPlaying(false);
    setCurrentTime(0);
  };

  const updateChannelSections = (
    channel: 'red' | 'green' | 'blue',
    updater: (sections: EquationSection[]) => EquationSection[]
  ) => {
    setPattern(prev => ({
      ...prev,
      [channel]: {
        ...prev[channel],
        sections: updater(prev[channel].sections),
      },
    }));
  };

  const addSection = (channel: 'red' | 'green' | 'blue') => {
    updateChannelSections(channel, sections => [
      ...sections,
      {
        id: crypto.randomUUID(),
        equation: '0',
        duration: 1000,
      },
    ]);
  };

  const updateSection = (
    channel: 'red' | 'green' | 'blue',
    id: string,
    updates: Partial<EquationSection>
  ) => {
    updateChannelSections(channel, sections =>
      sections.map(s => (s.id === id ? { ...s, ...updates } : s))
    );
  };

  const deleteSection = (channel: 'red' | 'green' | 'blue', id: string) => {
    updateChannelSections(channel, sections => sections.filter(s => s.id !== id));
  };

  const moveSection = (
    channel: 'red' | 'green' | 'blue',
    id: string,
    direction: 'up' | 'down'
  ) => {
    updateChannelSections(channel, sections => {
      const index = sections.findIndex(s => s.id === id);
      if (index === -1) return sections;
      if (direction === 'up' && index === 0) return sections;
      if (direction === 'down' && index === sections.length - 1) return sections;

      const newSections = [...sections];
      const swapIndex = direction === 'up' ? index - 1 : index + 1;
      [newSections[index], newSections[swapIndex]] = [newSections[swapIndex], newSections[index]];
      return newSections;
    });
  };

  return (
    <div className="flex flex-col gap-6 p-4 bg-gray-900 text-gray-100 rounded-lg shadow-xl">
      <div className="flex justify-between items-center">
        <h2 className="text-xl font-bold">Equation RGB Pattern Editor</h2>
        <div className="flex gap-2">
          <button
            onClick={handlePlayPause}
            className={`px-4 py-2 rounded font-bold ${
              isPlaying ? 'bg-yellow-600 hover:bg-yellow-500' : 'bg-green-600 hover:bg-green-500'
            }`}
          >
            {isPlaying ? 'Pause' : 'Play'}
          </button>
          <button
            onClick={handleStop}
            className="px-4 py-2 bg-gray-700 hover:bg-gray-600 rounded font-bold"
          >
            Stop
          </button>
        </div>
      </div>

      <div className="bg-gray-800/50 p-3 rounded text-sm text-gray-400 border border-gray-700">
        <p><strong>Equation Help:</strong> Use <code>t</code> for time (seconds) and <code>Duration</code> for section duration (seconds). Standard Math functions are available (e.g., <code>sin(t)</code>, <code>cos(t)</code>, <code>exp(t)</code>, <code>abs(t)</code>). Output is clamped to 0-255.</p>
        <p className="mt-1 text-xs">Examples: <code>255 * abs(sin(t))</code>, <code>255 * (t / Duration)</code>, <code>255 * exp(-t)</code></p>
      </div>

      {/* Preview Area */}
      <div className="bg-black rounded p-4 border border-gray-700">
        <h3 className="text-sm font-bold text-gray-400 mb-2">Combined Output Preview</h3>
        <ColorPreview
          redPoints={redPoints}
          greenPoints={greenPoints}
          bluePoints={bluePoints}
          currentTime={currentTime}
          totalDuration={totalDuration}
        />
        <div className="mt-2 text-right text-xs text-gray-500 font-mono">
          {(currentTime / 1000).toFixed(2)}s / {(totalDuration / 1000).toFixed(2)}s
        </div>
      </div>

      {/* Waveform Display Area */}
      <div className="grid grid-cols-1 gap-1 bg-black rounded p-4 border border-gray-700">
        <h3 className="text-sm font-bold text-gray-400 mb-2">Waveforms</h3>
        <WaveformLane
          color="red"
          points={redPoints}
          currentTime={currentTime}
          totalDuration={totalDuration}
        />
        <WaveformLane
          color="green"
          points={greenPoints}
          currentTime={currentTime}
          totalDuration={totalDuration}
        />
        <WaveformLane
          color="blue"
          points={bluePoints}
          currentTime={currentTime}
          totalDuration={totalDuration}
        />
      </div>

      {/* Section Management Area */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <SectionLane
          color="red"
          sections={pattern.red.sections}
          onAddSection={() => addSection('red')}
          onUpdateSection={(id, u) => updateSection('red', id, u)}
          onDeleteSection={id => deleteSection('red', id)}
          onMoveSection={(id, dir) => moveSection('red', id, dir)}
        />
        <SectionLane
          color="green"
          sections={pattern.green.sections}
          onAddSection={() => addSection('green')}
          onUpdateSection={(id, u) => updateSection('green', id, u)}
          onDeleteSection={id => deleteSection('green', id)}
          onMoveSection={(id, dir) => moveSection('green', id, dir)}
        />
        <SectionLane
          color="blue"
          sections={pattern.blue.sections}
          onAddSection={() => addSection('blue')}
          onUpdateSection={(id, u) => updateSection('blue', id, u)}
          onDeleteSection={id => deleteSection('blue', id)}
          onMoveSection={(id, dir) => moveSection('blue', id, dir)}
        />
      </div>
    </div>
  );
};
