# Casing TPU → PETG Manual Filament Change

Notes for printing the bottom casing with TPU first, then switching to PETG on an Ender 3 S1 Pro using one physical extruder.

## Goal

Print the lower flexible section in TPU, then pause after layer 3 and manually swap to PETG for the remaining layers.

## PrusaSlicer setup

Use two virtual filaments/extruders so PrusaSlicer generates the later layers with PETG temperatures, cooling, extrusion multiplier, and volumetric assumptions.

Current known settings from the working G-code:

- Filament 1 / virtual extruder 0: TPU
  - Profile: Eolas Prints TPU 93A @CREALITY
  - Nozzle temperature: 215°C
  - Bed temperature: 30°C
  - Max volumetric speed: 1.2 mm³/s
  - Extrusion multiplier: 1.16
- Filament 2 / virtual extruder 1: PETG
  - Profile: Generic PETG @CREALITY
  - Nozzle temperature: 240°C
  - Bed temperature: 70°C in profile, but the current print keeps bed at 30°C unless explicitly changed
  - Max volumetric speed: 8 mm³/s
  - Extrusion multiplier: 1.00
  - Cooling enabled, fan up to 50%

Important: the Ender 3 S1 Pro has one physical extruder. The slicer may emit `T1`, but for the stock single-extruder printer this can be unsafe or unsupported. In the hand-edited G-code, leave `T1` commented out and manually apply the PETG settings with explicit commands instead.

## Where to insert the manual change block

Insert the block **after layer 3**, at the layer change to **Z = 0.8 mm**.

In the generated G-code, look for this section:

```gcode
;LAYER_CHANGE
;Z:0.8
;HEIGHT:0.2
;BEFORE_LAYER_CHANGE
G92 E0
;0.8


G1 Z.8 F9000
;AFTER_LAYER_CHANGE
;0.8
```

Place the manual TPU → PETG block immediately after the `;AFTER_LAYER_CHANGE` / `;0.8` comments and before the next print move for the PETG layer.

This is the intended insertion point because:

- Z0.2 is layer 0 / first layer
- Z0.4 is layer 1
- Z0.6 is layer 2
- Z0.8 is layer 3 / the first PETG layer after the TPU base

## Manual TPU → PETG block

Current Ender 3 S1 Pro compatible block:

```gcode
;CUSTOM_GCODE
;--- TPU -> PETG MANUAL FILAMENT CHANGE ---
; This replaces the simple PrusaSlicer T1/M104 transition with a manual pause, purge, and return.
; Current layer is Z0.8, and the next toolpath resumes at X103.49 Y116.26.

;--- PREPARE FOR CHANGE ---
G1 E-2 F1800             ; Retract 2mm to stop TPU oozing before parking
G91                      ; Use relative positioning for the Z lift
G1 Z10 F3000             ; Lift nozzle 10mm above the part for clearance
G90                      ; Return to absolute positioning for XY/Z moves

;--- MOVE TO PURGE STATION ---
; T1                     ; Do not use on stock single-extruder Ender 3 S1 Pro
M106 S127.5              ; Apply PETG fan setting from slicer-generated toolchange block
G1 X0 Y220 F6000         ; Park at top-left/back-left purge position
G1 Z50 F3000             ; Lift to Z50 for easy filament swap and purge cleanup

;--- PAUSE FOR MANUAL FILAMENT CHANGE ---
M104 S240                ; Set PETG temperature before pause, but do not wait yet
M25                      ; Pause SD print for Ender 3 S1 Pro resume menu

;--- RESUME AFTER PETG IS LOADED ---
M104 S240                ; Re-assert PETG target in case the pause cleared it
M109 S240                ; Wait until the nozzle is back at PETG temperature before purging
M83                      ; Re-assert relative extrusion mode after pause/resume
G92 E0                   ; Reset relative extruder distance before purge
G1 E30 F200              ; Purge 30mm PETG slowly at Z50 to clear old TPU from nozzle
G92 E0                   ; Reset extruder distance after purge
G1 E-1 F1800             ; Retract 1mm to reduce PETG drip before returning to the part

;--- RETURN TO PRINT ---
G1 X103.49 Y116.26 F9000 ; Return to the next planned print position
G1 Z0.8 F3000            ; Lower back to the active layer height
G1 E0.8 F1800            ; Prime PETG after the retract
G1 Z.8 F9000             ; Match slicer Z command before continuing
G1 X103.49 Y116.26       ; Match slicer XY command before continuing
;--- END TPU -> PETG MANUAL FILAMENT CHANGE ---
```

## Why each important command is used

- `M25` is used instead of `M0` because the Ender 3 S1 Pro resume menu works with SD-print pause/resume. `M0` may stop without a usable resume path on Creality touch-screen firmware.
- `M104 S240` before `M25` requests PETG temperature before the manual swap.
- `M104 S240` after resume re-asserts the hotend target because the firmware pause may drop the target temperature to 0.
- `M109 S240` after resume waits until the nozzle is actually back to PETG temperature before purge/print continues.
- `M83` is important because the file is sliced for relative extrusion. Pause/resume can disturb modal state, so this prevents PETG moves from being interpreted as absolute extrusion.
- `G92 E0` resets the extruder distance around the purge so the following relative extrusion remains predictable.
- `G1 E30 F200` purges PETG slowly. `F200` is 200 mm/min, or 3.33 mm/s of filament feed. Increase this only if the extruder can push PETG reliably.

## Current caveats

- The bed remains at the TPU bed temperature unless a bed command is added. Add `M140 S70` / `M190 S70` only if the PETG section needs a hotter bed and the TPU base can tolerate it.
- A single physical extruder does not need active `T1`; keep it commented unless the firmware is known to support virtual tools correctly.
- If TPU remains in the nozzle, increase the PETG purge amount from 30mm to a larger value.
