# Flow Art Forge

This project represents a Web app that is used to configure LED flashing patterns on a PCB. Each Mode can have different patterns associated with it.

This json represents a mode that controls two leds. The 'front' LED is binary on/off and the 'case' led is represented by hex colors. It also changes to a different set of patterns when the accelerometer detects faster movement.
The duration and when each change happens is represented in milliseconds (ms).
```
{
  "mode": {
    "name": "accel Pattern",
    "front": {
      "pattern": {
        "name": "front binary pattern",
        "type": "simple",
        "duration": 3,
        "changeAt": [
          {
            "ms": 0,
            "output": "high"
          },
          {
            "ms": 2,
            "output": "low"
          }
        ]
      }
    },
    "case": {
      "pattern": {
        "name": "case color pattern",
        "type": "simple",
        "duration": 3,
        "changeAt": [
          {
            "ms": 0,
            "output": "#ffaa00"
          },
          {
            "ms": 2,
            "output": "#005110"
          }
        ]
      }
    },
    "accel": {
      "triggers": [
        {
          "threshold": 2,
          "front": {
            "pattern": {
              "name": "fullOn",
              "type": "simple",
              "duration": 1,
              "changeAt": [
                {
                  "ms": 0,
                  "output": "high"
                }
              ]
            }
          },
          "case": {
            "pattern": {
              "name": "flash white",
              "type": "simple",
              "duration": 3,
              "changeAt": [
                {
                  "ms": 0,
                  "output": "#ffffff"
                },
                {
                  "ms": 2,
                  "output": "#000000"
                }
              ]
            }
          }
        }
      ]
    }
  }
}
```

```
{
  "mode": {
    "name": "has rgb patterns for both",
    "front": {
      "pattern": {
        "name": "front color pattern",
        "type": "simple",
        "duration": 3,
        "changeAt": [
          {
            "ms": 0,
            "output": "#222222"
          },
          {
            "ms": 2,
            "output": "#773377"
          }
        ]
      }
    },
    "case": {
      "pattern": {
        "name": "case color pattern",
        "type": "simple",
        "duration": 3,
        "changeAt": [
          {
            "ms": 0,
            "output": "#ffaa00"
          },
          {
            "ms": 2,
            "output": "#005110"
          }
        ]
      }
    }
  }
}
```

Equation pattern example
```
{
  "mode": {
    "name": "equation mode",
    "front": {
      "pattern": {
        "type": "equation",
        "name": "sine wave",
        "duration": 1000,
        "red": {
          "sections": [
            {
              "equation": "127 + 127 * sin(2 * PI * t)",
              "duration": 1000
            }
          ],
          "loopAfterDuration": false
        },
        "green": {
          "sections": [],
          "loopAfterDuration": true
        },
        "blue": {
          "sections": [],
          "loopAfterDuration": true
        }
      }
    },
    "case": {
      "pattern": {
        "type": "simple",
        "name": "simple case",
        "duration": 100,
        "changeAt": [
          { "ms": 0, "output": "#000000" }
        ]
      }
    }
  }
}
```

Project Tools. Ensure the latest versions are used.
- TypeScript
- Vite
- React 19
- react-router-dom (HashRouter)
- tailwindcss (v4, CSS-first workflow)
- i18next, react-i18next, i18next-browser-languagedetector
- ESLint
- Prettier
- vitest
- zod
- zustand (only to be used when persisting something)

This will be hosted on vercel. 
Linting should be strict.

# components

Each component should be stateless. Props should have everything needed to show the state, and should include a callback that returns the current state, as well as what happened to change the state.

type Props = {
  value: ComponentState;
  onChange: (newState: ComponentState, action: ComponentAction) => void;
};

Each component should have unit tests to validate the behavior.
Each component should be properly localized with i18next useTranslation hook.
Each component should be themed.

# Features

### RGB pattern creation page

- select method of pattern creation
  - simple
    - Create patterns by defining a sequence of colors and durations.
  - equation
    - Create patterns using mathematical equations for Red, Green, and Blue channels.

### Bulb pattern creation page

- TODO

### Mode creation page

- Use created patterns to create a mode
  - TODO
- accelerometer can cause switch to a different pattern
  - TODO
- should be able save and load modes
  - TODO

### Settings page

- automatic theme selection based on system theme with manual override

### Serial log page

- shows all serial messages. 
  - Each message is separated by a new line with timestamp.
  - clear button
  - autoscroll toggle
- let you send custom payloads

This file should be updated with each change. DO NOT MAKE ASSUMPTIONS ON WHAT THE UI SHOULD LOOK LIKE FOR THINGS MARKED 'TODO'

## 2025-10-26

- Bootstrapped the Vite + React 19 + TypeScript workspace with Tailwind CSS v4, strict ESLint/Prettier, Vitest, and shared testing utilities.
- Added a global theme provider backed by zustand persistence, automatic system theme detection, and a Tailwind-powered layout with HashRouter navigation.
- Implemented translated placeholder pages for RGB patterns, bulb patterns, and mode composition while leaving TODO experiences untouched per guidance.
- Delivered a Settings page using a stateless `ThemePreferenceSelector` component that exposes value + action callbacks for switching between system, light, and dark themes.
- Built a Serial Log page featuring the stateless `SerialLogPanel` (autoscroll toggle, timestamped log view, clear button, outbound payload entry) with full localization coverage.
- Added Vitest suites for layout and component behavior to enforce the stateless contract and i18n wiring.
- Codified zod-backed data models for mode definitions (binary and RGB patterns, accelerometer triggers) with unit tests validating the documented JSON examples and failure modes.
- Delivered the simple RGB pattern creation flow with localized color/duration inputs, proportional preview visualization, removal controls, and dedicated component tests.
- Enhanced the simple RGB pattern flow with move, reorder, and duplicate controls plus expanded tests to validate the new interactions and maintain stateless behavior.
- Added inline color swatches beside each simple RGB step so the hex values are visually reinforced, along with tests ensuring every segment renders a preview chip.
- Reworked the simple RGB pattern panel to accept canonical `ModePattern` data, own draft form inputs locally, and emit schema-valid patterns with refreshed unit coverage.
- Introduced a persisted pattern library powered by zustand on the RGB Pattern page, including a method switcher UI, load/save controls, and overwrite protection when reusing names.
- Added a delete control for saved RGB patterns with confirmation prompts, plus expanded Vitest coverage for the page-level interactions and persistence store.
- Updated the simple RGB editor to default the segment duration to 250ms, restrict inputs to integers, and prevent saving patterns without at least one step, supported by refreshed unit tests.
- Refined the RGB pattern picker so selecting "New pattern" clears the editor, saving auto-selects the new entry, and accompanying tests ensure the dropdown mirrors the persisted library.
- Simplified the saved pattern workflow by removing the explicit load button—selections now hydrate the editor immediately, with updated tests confirming the auto-load flow.
- Redesigned the simple RGB step list with inline color and duration editors, wired through the new update-step action and validated by expanded unit coverage.

## 2025-12-12

- Implemented the Equation RGB pattern creation flow, allowing users to define patterns using mathematical formulas for Red, Green, and Blue channels.
- Added `EquationRgbPatternPanel` with interactive waveform visualization, playback controls, and section management (add, remove, reorder).
- Created `equation-evaluator` utility to safely evaluate user-defined mathematical expressions (e.g., `sin(t)`, `exp(t)`) for pattern generation.
- Integrated the equation editor into the `RgbPatternPage`, enabling switching between Simple and Equation methods with state persistence.
- Added `SectionLane`, `WaveformLane`, and `ColorPreview` components to support the equation editor UI.
- Validated the equation logic and components with comprehensive unit tests.
