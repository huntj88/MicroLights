import { useEffect, useMemo, useState, useCallback } from 'react';

interface UseEntityEditorProps<T extends { name: string }> {
  availableNames: string[];
  readItem: (name: string) => T | undefined;
  saveItem: (item: T) => void;
  deleteItem: (name: string) => void;
  createDefault: () => T;
  validate: (item: T) => string[];
  confirmOverwrite?: (name: string) => boolean;
  confirmDelete?: (name: string) => boolean;
}

export function useEntityEditor<T extends { name: string }>({
  availableNames,
  readItem,
  saveItem,
  deleteItem,
  createDefault,
  validate,
  confirmOverwrite,
  confirmDelete,
}: UseEntityEditorProps<T>) {
  const [selectedName, setSelectedName] = useState('');
  const [editingItem, setEditingItem] = useState<T>(createDefault);
  const [originalItem, setOriginalItem] = useState<T | null>(null);
  const [validationErrors, setValidationErrors] = useState<string[]>(() => {
    return validate(createDefault());
  });

  const handleSelect = useCallback(
    (name: string) => {
      setSelectedName(name);
      if (name) {
        const item = readItem(name);
        if (item) {
          setEditingItem(item);
          setOriginalItem(item);
          setValidationErrors(validate(item));
        }
      } else {
        const newItem = createDefault();
        setEditingItem(newItem);
        setOriginalItem(null);
        setValidationErrors(validate(newItem));
      }
    },
    [readItem, createDefault, validate],
  );

  const handleUpdate = useCallback(
    (newItem: T) => {
      setEditingItem(newItem);
      setValidationErrors(validate(newItem));

      // If the name changes to something that isn't the selected name,
      // we are effectively "detaching" from the saved item,
      // but we don't want to clear the form.

      // We can handle this by checking if the name matches selectedName.
    },
    [validate],
  );

  // We need a way to detect "rename" that detaches from original
  useEffect(() => {
    if (selectedName && editingItem.name !== selectedName) {
      // User changed the name of a selected item.
      // It is no longer "the item from storage".
      setSelectedName('');
      setOriginalItem(null);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [editingItem.name]);

  const handleSave = useCallback(() => {
    if (editingItem.name !== selectedName && availableNames.includes(editingItem.name)) {
      const confirmed = confirmOverwrite
        ? confirmOverwrite(editingItem.name)
        : confirm(`Overwrite existing item "${editingItem.name}"?`);

      if (!confirmed) return;
    }

    saveItem(editingItem);
    setSelectedName(editingItem.name);
    setOriginalItem(editingItem);
  }, [editingItem, selectedName, availableNames, saveItem, confirmOverwrite]);

  const handleDelete = useCallback(() => {
    if (!selectedName) return;

    const confirmed = confirmDelete
      ? confirmDelete(selectedName)
      : confirm('Are you sure you want to delete this item?');

    if (confirmed) {
      deleteItem(selectedName);
      handleSelect('');
    }
  }, [selectedName, confirmDelete, deleteItem, handleSelect]);

  const isDirty = useMemo(() => {
    if (!selectedName || !originalItem) {
      return true;
    }
    return JSON.stringify(editingItem) !== JSON.stringify(originalItem);
  }, [selectedName, originalItem, editingItem]);

  const isValid = validationErrors.length === 0;

  return {
    selectedName,
    setSelectedName: handleSelect,
    editingItem,
    setEditingItem: handleUpdate,
    validationErrors,
    isDirty,
    isValid,
    save: handleSave,
    remove: handleDelete,
  };
}
