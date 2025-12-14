import { type ChangeEvent } from 'react';

import { StyledButton } from './StyledButton';

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
          <StyledButton variant="danger" disabled={!selectedItem} onClick={onDelete}>
            {deleteLabel}
          </StyledButton>
          <StyledButton variant="primary" disabled={!isValid || !isDirty} onClick={onSave}>
            {saveLabel}
          </StyledButton>
        </div>
      </div>
    </div>
  );
};
