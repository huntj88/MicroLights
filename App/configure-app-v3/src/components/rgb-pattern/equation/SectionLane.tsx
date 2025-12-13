import { useTranslation } from 'react-i18next';

import type { EquationSection } from '../../../app/models/mode';

interface SectionLaneProps {
  color: 'red' | 'green' | 'blue';
  sections: EquationSection[];
  loopAfterDuration: boolean;
  onToggleLoop: (loop: boolean) => void;
  onAddSection: () => void;
  onUpdateSection: (index: number, updates: Partial<EquationSection>) => void;
  onDeleteSection: (index: number) => void;
  onMoveSection: (index: number, direction: 'up' | 'down') => void;
}

export const SectionLane = ({
  color,
  sections,
  loopAfterDuration,
  onToggleLoop,
  onAddSection,
  onUpdateSection,
  onDeleteSection,
  onMoveSection,
}: SectionLaneProps) => {
  const { t } = useTranslation();
  const borderColor = {
    red: 'border-red-500/50',
    green: 'border-green-500/50',
    blue: 'border-blue-500/50',
  }[color];

  return (
    <div
      className={`flex flex-col gap-2 p-2 rounded-xl border ${borderColor} bg-[rgb(var(--surface-raised)/0.3)]`}
    >
      <div className="flex justify-between items-center">
        <h3 className="text-sm font-bold uppercase theme-muted">
          {t('rgbPattern.equation.sections.title', {
            color: t(`rgbPattern.equation.colors.${color}`),
          })}
        </h3>
        <div className="flex items-center gap-2">
          <label
            className="flex items-center gap-1 text-xs theme-muted cursor-pointer hover:text-[rgb(var(--surface-contrast)/1)] transition-colors"
            title={t(
              'rgbPattern.equation.sections.loopTooltip',
              'Loop back to first section after last section finishes',
            )}
          >
            <input
              type="checkbox"
              checked={loopAfterDuration}
              onChange={e => {
                onToggleLoop(e.target.checked);
              }}
              className="rounded bg-[rgb(var(--surface-raised)/1)] theme-border border text-[rgb(var(--accent)/1)] focus:ring-[rgb(var(--accent)/1)] focus:ring-offset-[rgb(var(--surface)/1)]"
            />
            {t('rgbPattern.equation.sections.loop', 'Loop')}
          </label>
          <button
            onClick={onAddSection}
            className="px-2 py-1 text-xs bg-[rgb(var(--surface-raised)/1)] hover:bg-[rgb(var(--surface-raised)/0.8)] rounded text-[rgb(var(--surface-contrast)/1)] transition-colors"
          >
            {t('rgbPattern.equation.sections.add')}
          </button>
        </div>
      </div>

      <div className="flex flex-col gap-2">
        {sections.map((section, index) => (
          <SectionItem
            key={index}
            section={section}
            index={index}
            total={sections.length}
            onUpdate={updates => {
              onUpdateSection(index, updates);
            }}
            onDelete={() => {
              onDeleteSection(index);
            }}
            onMove={dir => {
              onMoveSection(index, dir);
            }}
          />
        ))}
        {sections.length === 0 && (
          <div className="text-center py-4 theme-muted text-xs italic">
            {t('rgbPattern.equation.sections.empty')}
          </div>
        )}
      </div>
    </div>
  );
};

interface SectionItemProps {
  section: EquationSection;
  index: number;
  total: number;
  onUpdate: (updates: Partial<EquationSection>) => void;
  onDelete: () => void;
  onMove: (dir: 'up' | 'down') => void;
}

const SectionItem = ({ section, index, total, onUpdate, onDelete, onMove }: SectionItemProps) => {
  const { t } = useTranslation();

  return (
    <div className="bg-[rgb(var(--surface-raised)/0.5)] rounded-lg p-2 flex flex-col gap-2 border theme-border">
      <div className="flex justify-between items-start gap-2">
        <div className="flex-1 flex flex-col gap-1">
          <label className="text-xs theme-muted">
            {t('rgbPattern.equation.sections.equationLabel')}
          </label>
          <input
            type="text"
            value={section.equation}
            onChange={e => {
              onUpdate({ equation: e.target.value });
            }}
            className="w-full bg-[rgb(var(--surface)/0.5)] theme-border border rounded px-2 py-1 text-sm font-mono text-[rgb(var(--accent)/1)] focus:outline-none focus:border-[rgb(var(--accent)/1)]"
            placeholder={t('rgbPattern.equation.sections.equationPlaceholder')}
          />
        </div>
        <div className="flex flex-col gap-1 w-24">
          <label className="text-xs theme-muted">
            {t('rgbPattern.equation.sections.durationLabel')}
          </label>
          <input
            type="number"
            value={Number.isNaN(section.duration) ? '' : section.duration}
            onChange={e => {
              const val = parseInt(e.target.value);
              onUpdate({ duration: Number.isNaN(val) ? NaN : val });
            }}
            className="w-full bg-[rgb(var(--surface)/0.5)] theme-border border rounded px-2 py-1 text-sm text-[rgb(var(--surface-contrast)/1)] focus:outline-none focus:border-[rgb(var(--accent)/1)]"
          />
        </div>
      </div>

      <div className="flex justify-end gap-2 mt-1">
        <button
          onClick={() => {
            onMove('up');
          }}
          disabled={index === 0}
          className="theme-muted hover:text-[rgb(var(--surface-contrast)/1)] disabled:opacity-30 transition-colors"
          title={t('rgbPattern.equation.sections.moveUp')}
        >
          ↑
        </button>
        <button
          onClick={() => {
            onMove('down');
          }}
          disabled={index === total - 1}
          className="theme-muted hover:text-[rgb(var(--surface-contrast)/1)] disabled:opacity-30 transition-colors"
          title={t('rgbPattern.equation.sections.moveDown')}
        >
          ↓
        </button>
        <div className="w-px bg-[rgb(var(--surface-muted)/0.3)] mx-1"></div>
        <button
          onClick={onDelete}
          className="text-red-500 hover:text-red-400 text-xs transition-colors"
        >
          {t('rgbPattern.equation.sections.delete')}
        </button>
      </div>
    </div>
  );
};
