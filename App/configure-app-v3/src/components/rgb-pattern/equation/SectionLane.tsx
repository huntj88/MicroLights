import type { EquationSection } from '../../../app/models/equation-pattern';

interface SectionLaneProps {
  color: 'red' | 'green' | 'blue';
  sections: EquationSection[];
  onAddSection: () => void;
  onUpdateSection: (id: string, updates: Partial<EquationSection>) => void;
  onDeleteSection: (id: string) => void;
  onMoveSection: (id: string, direction: 'up' | 'down') => void;
}

export const SectionLane = ({
  color,
  sections,
  onAddSection,
  onUpdateSection,
  onDeleteSection,
  onMoveSection,
}: SectionLaneProps) => {
  const borderColor = {
    red: 'border-red-500/50',
    green: 'border-green-500/50',
    blue: 'border-blue-500/50',
  }[color];

  return (
    <div className={`flex flex-col gap-2 p-2 rounded border ${borderColor} bg-gray-800/30`}>
      <div className="flex justify-between items-center">
        <h3 className="text-sm font-bold uppercase text-gray-400">{color} Sections</h3>
        <button
          onClick={onAddSection}
          className="px-2 py-1 text-xs bg-gray-700 hover:bg-gray-600 rounded text-white"
        >
          + Add Section
        </button>
      </div>

      <div className="flex flex-col gap-2">
        {sections.map((section, index) => (
          <SectionItem
            key={section.id}
            section={section}
            index={index}
            total={sections.length}
            onUpdate={updates => { onUpdateSection(section.id, updates); }}
            onDelete={() => { onDeleteSection(section.id); }}
            onMove={dir => { onMoveSection(section.id, dir); }}
          />
        ))}
        {sections.length === 0 && (
          <div className="text-center py-4 text-gray-500 text-xs italic">
            No sections defined.
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
  return (
    <div className="bg-gray-700 rounded p-2 flex flex-col gap-2">
      <div className="flex justify-between items-start gap-2">
        <div className="flex-1 flex flex-col gap-1">
          <label className="text-xs text-gray-400">Equation (t = time in sec)</label>
          <input
            type="text"
            value={section.equation}
            onChange={e => { onUpdate({ equation: e.target.value }); }}
            className="w-full bg-gray-900 border border-gray-600 rounded px-2 py-1 text-sm font-mono text-green-400"
            placeholder="e.g. 255 * sin(t)"
          />
        </div>
        <div className="flex flex-col gap-1 w-24">
          <label className="text-xs text-gray-400">Duration (ms)</label>
          <input
            type="number"
            value={section.duration}
            onChange={e => { onUpdate({ duration: parseInt(e.target.value) || 0 }); }}
            className="w-full bg-gray-900 border border-gray-600 rounded px-2 py-1 text-sm"
          />
        </div>
      </div>
      
      <div className="flex justify-end gap-2 mt-1">
        <button
          onClick={() => { onMove('up'); }}
          disabled={index === 0}
          className="text-gray-400 hover:text-white disabled:opacity-30"
          title="Move Up"
        >
          ↑
        </button>
        <button
          onClick={() => { onMove('down'); }}
          disabled={index === total - 1}
          className="text-gray-400 hover:text-white disabled:opacity-30"
          title="Move Down"
        >
          ↓
        </button>
        <div className="w-px bg-gray-600 mx-1"></div>
        <button
          onClick={onDelete}
          className="text-red-400 hover:text-red-300 text-xs"
        >
          Delete
        </button>
      </div>
    </div>
  );
};
