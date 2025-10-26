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
    - TODO
  - equation
    - TODO

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
