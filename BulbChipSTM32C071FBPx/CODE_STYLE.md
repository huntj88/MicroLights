Code style and conventions — based on chip_state.c

This document records the coding conventions observed in Core/Src/chip_state.c and proposes sensible additions and tightening for the repository. Follow the existing patterns unless you accept an explicit change across the codebase (preferably via a single follow-up PR with automated formatting).

1) File layout and top-of-file header
- Each .c/.h should start with a short header comment block that includes at least: filename, creation/author information and a short description.
- Order within a file:
  1. File-level comment / metadata
  2. Includes (ordered: project headers, library headers, system headers)
  3. Static (file-scope) variables and constants
  4. Static (internal) helper functions
  5. Public/exported functions
  6. Interrupt handlers and ISRs near the bottom (if present)
- Keep related helper functions grouped together and place static helpers before public functions that call them.

2) Includes and header ordering
- Use the order: local project headers first ("chip_state.h"), then module headers, then 3rd-party or system headers (<stdint.h>, <stdbool.h>, <string.h>, ...).
- Prefer including only what a file needs.

3) Naming conventions
- Types (structs, enums, typedefs): PascalCase (e.g., BulbMode, ChipSettings, BQ25180).
- Functions: lowerCamelCase (e.g., configureChipState, readBulbModeWithBuffer, handleButtonInput).
- File-scope/static variables: lowerCamelCase (e.g., modeCount, clickStarted, fakeOffModeIndex).
- Constants: repository currently uses file-scope const with lowerCamelCase. For new macros/compile-time constants prefer UPPER_SNAKE_CASE if they are preprocessor macros; keep const variables as lowerCamelCase.
- Boolean flags: use descriptive names and explicitly type as bool or volatile bool when used across ISRs.

4) Static / file-scope usage
- Use static for all symbols that are local to a single translation unit.
- Prefer explicitly marking variables used by both ISRs and foreground code as volatile and document why (e.g., ticksSinceLastUserActivity is volatile).
- Keep the public API (functions used outside the file) declared in the corresponding header and implemented without the static keyword.

5) Function structure and size
- Keep functions focused and small. If a function grows large, split into logically named static helpers.
- Prefer descriptive names for handler functions (e.g., showChargingState) and keep their responsibilities narrow.
- Interrupt handlers should be short and defer work to the main loop or a scheduled task when possible (the file follows this pattern: e.g., handleChargerInterrupt sets a flag).

6) Error handling and fallbacks
- Provide sensible fallbacks for resource/read failures (see readBulbModeWithBuffer which falls back to a default mode when flash read fails).
- Log (via writeUsbSerial) useful diagnostic messages when parsing or hardware operations fail.

7) Memory usage and stack safety
- Embedded environments have small stacks. Large temporary buffers (e.g., char flashReadBuffer[1024]) are acceptable in existing code, however:
  - Prefer the heap-free approach used here, but keep buffer sizes conservative. Document the stack budget per thread/interrupt and avoid very large automatic arrays where possible.
  - Consider passing caller-provided buffers into functions to avoid repeated large allocations on the stack.

8) Comments and documentation
- Short block header for each file.
- Use short inline comments to explain non-obvious logic (e.g., timing math, magic numbers). chip_state.c uses comments for timing frequency and thresholds — keep those.
- For public functions, either provide a brief comment in the header file or document behaviour in the .c above the function.

9) Timing and units
- Be explicit about tick units everywhere. The repo uses custom tick counters (e.g., 0.1 Hz auto-off timer) and arithmetic based on those ticks. Always document the units (ticks per second, tick frequency) where a variable or function depends on them.
- If multiple modules interact using ticks, define a central convention (e.g., OS_TICKS_PER_SECOND or TICKS_PER_0_1HZ) to avoid accidental mismatches.

10) Concurrency and ISR-safe patterns
- Mark shared flags/variables used in ISRs as volatile.
- Keep ISRs minimal: set flags and return. Defer heavy operations to tasks called from the main loop.
- Use atomic or critical sections when reading/writing multi-word shared variables if the platform needs it.

11) Logging and diagnostics
- Use an existing writeUsbSerial function for lightweight diagnostics; prefer short, structured messages (JSON or newline-terminated small strings) to make parsing easier.
- Protect log buffers and avoid long blocking logs inside ISRs.

12) JSON parsing and input validation
- The code uses parseJson and CliInput to decode JSON; always validate the parse result before using parsed data (chip_state.c does this). Keep that pattern and log parse errors.

13) TODOs and temporary code
- Keep TODO comments with a short description and, when relevant, a label like TODO(#issue) or FIXME so they can be discovered by tooling.
