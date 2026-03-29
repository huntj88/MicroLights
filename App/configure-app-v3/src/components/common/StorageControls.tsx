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
    <div className="flex flex-col gap-3 sm:flex-row sm:flex-wrap sm:items-end sm:justify-between sm:gap-4 border-b theme-border pb-4 sm:pb-6 w-full">
      <div className="flex flex-1 flex-col sm:flex-row sm:flex-wrap items-stretch sm:items-end gap-3 sm:gap-4">
        <label className="flex flex-col gap-1.5 flex-1 sm:max-w-sm">
          <span className="text-sm font-medium text-[rgb(var(--surface-contrast)/0.8)]">
            {selectLabel}
          </span>
          <select
            className="h-11 rounded-xl border theme-border bg-[rgb(var(--surface-raised)/0.5)] px-3 text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none w-full"
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

        <div className="flex w-full gap-2 sm:w-auto">
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
