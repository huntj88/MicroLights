#ifndef MODE_STATE_H
#define MODE_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "model/mode.h"
#include "tinyexpr.h"

typedef struct {
    uint32_t elapsedMs;
    uint8_t changeIndex;
} SimplePatternState;

typedef struct {
    uint8_t currentSectionIndex;
    uint32_t sectionElapsedMs;
    te_expr *compiledExprs[3];
    float t_var;
} EquationChannelState;

typedef struct {
    uint32_t elapsedMs;
    EquationChannelState red;
    EquationChannelState green;
    EquationChannelState blue;
} EquationPatternState;

typedef struct {
    SimplePatternState simple;
    EquationPatternState equation;
} ModeComponentState;

typedef struct {
    ModeComponentState front;
    ModeComponentState case_comp;
} ModeAccelTriggerState;

enum { MODE_ACCEL_TRIGGER_MAX = sizeof(((ModeAccel *)0)->triggers) / sizeof(ModeAccelTrigger) };

typedef struct {
    ModeComponentState front;
    ModeComponentState case_comp;
    ModeAccelTriggerState accel[MODE_ACCEL_TRIGGER_MAX];
    uint32_t lastPatternUpdateMs;
} ModeState;

void modeStateReset(ModeState *state, const Mode *mode, uint32_t initialMs);
void modeStateAdvance(ModeState *state, const Mode *mode, uint32_t milliseconds);
bool modeStateGetSimpleOutput(
    const ModeComponentState *componentState, const ModeComponent *component, SimpleOutput *output);

#endif /* MODE_STATE_H */
