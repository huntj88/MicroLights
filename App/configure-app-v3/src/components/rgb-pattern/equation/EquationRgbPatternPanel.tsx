import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { Trans, useTranslation } from 'react-i18next';

import { ColorPreview } from './ColorPreview';
import { SectionLane } from './SectionLane';
import { WaveformLane } from './WaveformLane';
import { type EquationPattern, type EquationSection } from '../../../app/models/mode';
import { generateWaveformPoints } from '../../../utils/equation-evaluator';

export type EquationRgbPatternAction =
  | { type: 'rename-pattern'; name: string }
  | { type: 'add-section'; channel: 'red' | 'green' | 'blue'; section: EquationSection }
  | {
      type: 'update-section';
      channel: 'red' | 'green' | 'blue';
      index: number;
      section: EquationSection;
    }
  | { type: 'remove-section'; channel: 'red' | 'green' | 'blue'; index: number }
  | { type: 'move-section'; channel: 'red' | 'green' | 'blue'; fromIndex: number; toIndex: number }
  | {
      type: 'update-channel-config';
      channel: 'red' | 'green' | 'blue';
      loopAfterDuration: boolean;
    };

export interface EquationRgbPatternPanelProps {
  pattern: EquationPattern;
  onChange: (state: EquationPattern, action: EquationRgbPatternAction) => void;
}

export const EquationRgbPatternPanel = ({ pattern, onChange }: EquationRgbPatternPanelProps) => {
  const { t } = useTranslation();
  const [isPlaying, setIsPlaying] = useState(false);
  const [currentTime, setCurrentTime] = useState(0);
  const requestRef = useRef<number>(0);
  const lastTimeRef = useRef<number>(0);

  const calculateChannelDuration = (sections: EquationSection[]) =>
    sections.reduce(
      (acc, section) => acc + (Number.isNaN(section.duration) ? 0 : section.duration),
      0,
    );

  const redDuration = useMemo(
    () => calculateChannelDuration(pattern.red.sections),
    [pattern.red.sections],
  );
  const greenDuration = useMemo(
    () => calculateChannelDuration(pattern.green.sections),
    [pattern.green.sections],
  );
  const blueDuration = useMemo(
    () => calculateChannelDuration(pattern.blue.sections),
    [pattern.blue.sections],
  );

  const totalDuration = Math.max(redDuration, greenDuration, blueDuration, 1000); // Min 1s for display

  // Generate waveforms
  // We generate points for the entire duration.
  // Sample rate 10ms is fine for display.
  const redPoints = useMemo(
    () =>
      generateWaveformPoints(pattern.red.sections, totalDuration, pattern.red.loopAfterDuration),
    [pattern.red.sections, totalDuration, pattern.red.loopAfterDuration],
  );
  const greenPoints = useMemo(
    () =>
      generateWaveformPoints(
        pattern.green.sections,
        totalDuration,
        pattern.green.loopAfterDuration,
      ),
    [pattern.green.sections, totalDuration, pattern.green.loopAfterDuration],
  );
  const bluePoints = useMemo(
    () =>
      generateWaveformPoints(pattern.blue.sections, totalDuration, pattern.blue.loopAfterDuration),
    [pattern.blue.sections, totalDuration, pattern.blue.loopAfterDuration],
  );

  const animate = useCallback(
    (time: number) => {
      if (lastTimeRef.current === 0) {
        lastTimeRef.current = time;
      }

      const deltaTime = time - lastTimeRef.current;
      lastTimeRef.current = time;

      setCurrentTime(prev => {
        const next = prev + deltaTime;
        if (next >= totalDuration) {
          // Loop or stop? loop for now in the preview.
          return 0;
        }
        return next;
      });

      requestRef.current = requestAnimationFrame(animate);
    },
    [totalDuration],
  );

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
  }, [animate, isPlaying, totalDuration]);

  const handlePlayPause = () => {
    setIsPlaying(!isPlaying);
  };

  const handleStop = () => {
    setIsPlaying(false);
    setCurrentTime(0);
  };

  const handleNameChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const nextName = event.target.value;
    const nextPattern = { ...pattern, name: nextName };
    onChange(nextPattern, { type: 'rename-pattern', name: nextName });
  };

  const addSection = (channel: 'red' | 'green' | 'blue') => {
    const newSection: EquationSection = {
      equation: '0',
      duration: 1000,
    };

    const nextSections = [...pattern[channel].sections, newSection];
    const nextPattern = {
      ...pattern,
      [channel]: {
        ...pattern[channel],
        sections: nextSections,
      },
    };

    onChange(nextPattern, { type: 'add-section', channel, section: newSection });
  };

  const updateSection = (
    channel: 'red' | 'green' | 'blue',
    index: number,
    updates: Partial<EquationSection>,
  ) => {
    const sections = pattern[channel].sections;
    if (index < 0 || index >= sections.length) return;

    const updatedSection = { ...sections[index], ...updates };
    const nextSections = [...sections];
    nextSections[index] = updatedSection;

    const nextPattern = {
      ...pattern,
      [channel]: {
        ...pattern[channel],
        sections: nextSections,
      },
    };

    onChange(nextPattern, { type: 'update-section', channel, index, section: updatedSection });
  };

  const deleteSection = (channel: 'red' | 'green' | 'blue', index: number) => {
    const nextSections = pattern[channel].sections.filter((_, i) => i !== index);

    const nextPattern = {
      ...pattern,
      [channel]: {
        ...pattern[channel],
        sections: nextSections,
      },
    };

    onChange(nextPattern, { type: 'remove-section', channel, index });
  };

  const moveSection = (
    channel: 'red' | 'green' | 'blue',
    index: number,
    direction: 'up' | 'down',
  ) => {
    const sections = pattern[channel].sections;
    if (index < 0 || index >= sections.length) return;
    if (direction === 'up' && index === 0) return;
    if (direction === 'down' && index === sections.length - 1) return;

    const newSections = [...sections];
    const swapIndex = direction === 'up' ? index - 1 : index + 1;
    [newSections[index], newSections[swapIndex]] = [newSections[swapIndex], newSections[index]];

    const nextPattern = {
      ...pattern,
      [channel]: {
        ...pattern[channel],
        sections: newSections,
      },
    };

    onChange(nextPattern, { type: 'move-section', channel, fromIndex: index, toIndex: swapIndex });
  };

  const updateChannelLoop = (channel: 'red' | 'green' | 'blue', loop: boolean) => {
    const nextPattern = {
      ...pattern,
      [channel]: {
        ...pattern[channel],
        loopAfterDuration: loop,
      },
    };
    onChange(nextPattern, { type: 'update-channel-config', channel, loopAfterDuration: loop });
  };

  return (
    <div className="flex flex-col gap-6 p-4 bg-gray-900 text-gray-100 rounded-lg shadow-xl">
      <div className="flex justify-between items-center">
        <div className="flex-1 mr-4">
          <label className="flex flex-col gap-1 text-sm">
            <span className="font-bold text-gray-400">
              {t('rgbPattern.equation.form.nameLabel', 'Pattern Name')}
            </span>
            <input
              className="w-full rounded bg-gray-800 border border-gray-700 px-3 py-2 text-white focus:border-blue-500 focus:outline-none"
              onChange={handleNameChange}
              placeholder={t('rgbPattern.equation.form.namePlaceholder', 'Enter pattern name')}
              type="text"
              value={pattern.name}
            />
          </label>
        </div>
        <div className="flex gap-2 self-end mb-1">
          <button
            onClick={handlePlayPause}
            className={`px-4 py-2 rounded font-bold ${
              isPlaying ? 'bg-yellow-600 hover:bg-yellow-500' : 'bg-green-600 hover:bg-green-500'
            }`}
          >
            {isPlaying
              ? t('rgbPattern.equation.controls.pause')
              : t('rgbPattern.equation.controls.play')}
          </button>
          <button
            onClick={handleStop}
            className="px-4 py-2 bg-gray-700 hover:bg-gray-600 rounded font-bold"
          >
            {t('rgbPattern.equation.controls.stop')}
          </button>
        </div>
      </div>

      <div className="bg-gray-800/50 p-3 rounded text-sm text-gray-400 border border-gray-700">
        <p>
          <strong>{t('rgbPattern.equation.help.title')}</strong>{' '}
          <Trans i18nKey="rgbPattern.equation.help.description">
            Use <code>t</code> for time (seconds) and <code>Duration</code> for section duration
            (seconds). Standard Math functions are available (e.g., <code>sin(t)</code>,{' '}
            <code>cos(t)</code>, <code>exp(t)</code>, <code>abs(t)</code>). Output is clamped to
            0-255.
          </Trans>
        </p>
        <p className="mt-1 text-xs">
          {t('rgbPattern.equation.help.examples')} <code>255 * abs(sin(t))</code>,{' '}
          <code>255 * (t / Duration)</code>, <code>255 * exp(-t)</code>
        </p>
      </div>

      {/* Preview Area */}
      <div className="bg-black rounded p-4 border border-gray-700">
        <h3 className="text-sm font-bold text-gray-400 mb-2">
          {t('rgbPattern.equation.preview.title')}
        </h3>
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
        <h3 className="text-sm font-bold text-gray-400 mb-2">
          {t('rgbPattern.equation.waveforms.title')}
        </h3>
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
          loopAfterDuration={pattern.red.loopAfterDuration}
          onToggleLoop={loop => {
            updateChannelLoop('red', loop);
          }}
          onAddSection={() => {
            addSection('red');
          }}
          onUpdateSection={(index, u) => {
            updateSection('red', index, u);
          }}
          onDeleteSection={index => {
            deleteSection('red', index);
          }}
          onMoveSection={(index, dir) => {
            moveSection('red', index, dir);
          }}
        />
        <SectionLane
          color="green"
          sections={pattern.green.sections}
          loopAfterDuration={pattern.green.loopAfterDuration}
          onToggleLoop={loop => {
            updateChannelLoop('green', loop);
          }}
          onAddSection={() => {
            addSection('green');
          }}
          onUpdateSection={(index, u) => {
            updateSection('green', index, u);
          }}
          onDeleteSection={index => {
            deleteSection('green', index);
          }}
          onMoveSection={(index, dir) => {
            moveSection('green', index, dir);
          }}
        />
        <SectionLane
          color="blue"
          sections={pattern.blue.sections}
          loopAfterDuration={pattern.blue.loopAfterDuration}
          onToggleLoop={loop => {
            updateChannelLoop('blue', loop);
          }}
          onAddSection={() => {
            addSection('blue');
          }}
          onUpdateSection={(index, u) => {
            updateSection('blue', index, u);
          }}
          onDeleteSection={index => {
            deleteSection('blue', index);
          }}
          onMoveSection={(index, dir) => {
            moveSection('blue', index, dir);
          }}
        />
      </div>
    </div>
  );
};
