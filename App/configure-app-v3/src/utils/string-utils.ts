/**
 * Converts a camelCase string into a human-readable title case string.
 * Handles acronyms intelligently by preserving consecutive capital letters.
 *
 * Examples:
 * - "camelCase" -> "Camel Case"
 * - "enableUSBSerial" -> "Enable USB Serial"
 * - "simple" -> "Simple"
 *
 * @param key - The camelCase string to format
 * @returns The formatted human-readable string
 */
export const humanizeCamelCase = (key: string): string => {
  return (
    key
      // Insert space before capital letters that are preceded by a lowercase letter
      .replace(/([a-z])([A-Z])/g, '$1 $2')
      // Insert space before capital letters that are followed by a lowercase letter (for acronyms ending)
      // e.g. "USBSerial" -> "USB Serial"
      .replace(/([A-Z])([A-Z][a-z])/g, '$1 $2')
      .replace(/^./, str => str.toUpperCase())
      .trim()
  );
};
