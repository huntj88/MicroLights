#ifndef MODE_STATE_H
#define MODE_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "model/mode.h"

typedef struct {
    uint32_t elapsedMs;
    uint8_t changeIndex;
} SimplePatternState;

typedef struct {
    SimplePatternState simple;
} ModeComponentState;

typedef struct {
    ModeComponentState front;
    ModeComponentState case_comp;
} ModeAccelTriggerState;

enum {
    MODE_ACCEL_TRIGGER_MAX = sizeof(((ModeAccel *)0)->triggers) / sizeof(ModeAccelTrigger)
};

typedef struct {
    ModeComponentState front;
    ModeComponentState case_comp;
    ModeAccelTriggerState accel[MODE_ACCEL_TRIGGER_MAX];
} ModeState;

void modeStateReset(ModeState *state);
void modeStateAdvance(ModeState *state, const Mode *mode, uint32_t deltaMs);
bool modeStateGetSimpleOutput(const ModeComponentState *componentState, const ModeComponent *component, SimpleOutput *output);

#endif /* MODE_STATE_H */
