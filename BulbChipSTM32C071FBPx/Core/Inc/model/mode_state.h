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
    te_expr *compiledExprs[CHANNEL_CONFIG_SECTIONS_MAX];
    float t_var;
    uint32_t lastEvalMs;
    uint8_t cachedOutput;
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

typedef struct {
    ModeComponentState front;
    ModeComponentState case_comp;
    ModeAccelTriggerState accel[MODE_ACCEL_TRIGGERS_MAX];
    uint32_t lastPatternUpdateMs;
} ModeState;

typedef struct {
    bool hasError;
    char path[128];
    int errorPosition;
    char equation[EQUATION_SECTION_EQUATION_MAX_LEN];
} ModeEquationError;

/**
 * Initializes a `ModeState` instance so it can evaluate the provided `Mode`.
 * Frees any previously compiled expressions, zeroes the runtime state, seeds
 * `lastPatternUpdateMs` with `initialMs`, and compiles all required equations.
 * Returns false and populates `error` when an equation fails to compile.
 */
bool modeStateInitialize(
    ModeState *state, const Mode *mode, uint32_t initialMs, ModeEquationError *error);
void modeStateAdvance(ModeState *state, const Mode *mode, uint32_t milliseconds);
bool modeStateGetSimpleOutput(
    const ModeComponentState *componentState,
    const ModeComponent *component,
    SimpleOutput *output,
    uint8_t equationEvalIntervalMs);

#ifdef UNIT_TEST
void modeStateTest_resetEquationFreeCounter(void);
uint32_t modeStateTest_getEquationFreeCounter(void);
#endif

#endif /* MODE_STATE_H */
