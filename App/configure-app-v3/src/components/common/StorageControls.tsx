import { type ChangeEvent } from 'react';

interface Props {
  // Selection
  items: string[];
  selectedItem: string;
  onSelect: (item: string) => void;
  selectLabel: string;
  selectPlaceholder: string;

  // Actions
  onSave: () => void;
  onDelete: () => void;

  // State
  isDirty?: boolean;
  isValid?: boolean;

  // Labels
  saveLabel: string;
  deleteLabel: string;
}

export const StorageControls = ({
  items,
  selectedItem,
  onSelect,
  selectLabel,
  selectPlaceholder,
  onSave,
  onDelete,
  isDirty = true,
  isValid = true,
  saveLabel,
  deleteLabel,
}: Props) => {
  const handleSelectChange = (e: ChangeEvent<HTMLSelectElement>) => {
    onSelect(e.target.value);
  };

  return (
    <div className="flex flex-wrap items-end justify-between gap-4 border-b theme-border pb-6 w-full">
      <div className="flex flex-1 flex-wrap items-end gap-4">
        <label className="flex flex-col gap-1.5 flex-1 max-w-sm">
          <span className="text-sm font-medium text-[rgb(var(--surface-contrast)/0.8)]">
            {selectLabel}
          </span>
          <select
            className="h-10 rounded-xl border theme-border bg-[rgb(var(--surface-raised)/0.5)] px-3 text-sm text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none w-full"
            onChange={handleSelectChange}
            value={selectedItem}
          >
            <option value="">{selectPlaceholder}</option>
            {items.map(item => (
              <option key={item} value={item}>
                {item}
              </option>
            ))}
          </select>
        </label>

        <div className="flex gap-2">
          <button
            className="rounded-full px-4 py-2 text-sm font-medium text-red-400 transition-colors hover:bg-red-500/10 disabled:opacity-50"
            disabled={!selectedItem}
            onClick={onDelete}
            type="button"
          >
            {deleteLabel}
          </button>
          <button
            className="rounded-full bg-[rgb(var(--accent)/1)] px-4 py-2 text-sm font-medium text-[rgb(var(--surface-contrast)/1)] transition-transform hover:scale-[1.01] disabled:opacity-50"
            disabled={!isValid || !isDirty}
            onClick={onSave}
            type="button"
          >
            {saveLabel}
          </button>
        </div>
      </div>
    </div>
  );
};
