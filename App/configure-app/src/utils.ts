export function cloneValue<T>(value: T): T {
  return typeof value == "object" ? JSON.parse(JSON.stringify(value)) : value;
}
