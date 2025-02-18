export function getFromLocalStorage<T>(key: string): T | undefined {
  const storedValue = localStorage.getItem(key);
  return storedValue ? (JSON.parse(storedValue) as T) : undefined;
}
