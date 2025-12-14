import type { ChangeEvent } from 'react';

interface NameEditorProps {
  name: string;
  onChange: (name: string) => void;
  label: string;
  placeholder: string;
  helperText: string;
}

export const NameEditor = ({
  name,
  onChange,
  label,
  placeholder,
  helperText,
}: NameEditorProps) => {
  const handleChange = (event: ChangeEvent<HTMLInputElement>) => {
    onChange(event.target.value);
  };

  return (
    <label className="flex flex-col gap-1 text-sm">
      <span className="font-bold theme-muted">{label}</span>
      <input
        className="w-full rounded-xl bg-[rgb(var(--surface-raised)/0.5)] theme-border border px-3 py-2 text-[rgb(var(--surface-contrast)/1)] focus:border-[rgb(var(--accent)/1)] focus:outline-none transition-colors"
        onChange={handleChange}
        placeholder={placeholder}
        type="text"
        value={name}
      />
      {helperText && <span className="text-xs theme-muted">{helperText}</span>}
    </label>
  );
};
