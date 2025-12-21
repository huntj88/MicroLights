#include "model/mode_state.h"

#include <string.h>

static void advanceSimplePattern(SimplePatternState *state, const SimplePattern *pattern, uint32_t deltaMs) {
    if (!state || !pattern || pattern->changeAt_count == 0U || deltaMs == 0U) {
        if (state && pattern && pattern->changeAt_count > 0U && state->changeIndex >= pattern->changeAt_count) {
            state->changeIndex = pattern->changeAt_count - 1U;
        }
        return;
    }

    uint32_t duration = pattern->duration;
    if (duration == 0U) {
        state->elapsedMs = 0U;
        state->changeIndex = 0U;
        return;
    }

    uint32_t elapsed = state->elapsedMs + deltaMs;
    while (elapsed >= duration) {
        elapsed -= duration;
        state->changeIndex = 0U;
    }

    state->elapsedMs = elapsed;

    while ((state->changeIndex + 1U) < pattern->changeAt_count &&
           pattern->changeAt[state->changeIndex + 1U].ms <= state->elapsedMs) {
        state->changeIndex++;
    }

    while (state->changeIndex > 0U &&
           pattern->changeAt[state->changeIndex].ms > state->elapsedMs) {
        state->changeIndex--;
    }
}

static void advanceComponentState(ModeComponentState *componentState, const ModeComponent *component, uint32_t deltaMs) {
    if (!componentState || !component) {
        return;
    }

    if (component->pattern.type != PATTERN_TYPE_SIMPLE) {
        componentState->simple.elapsedMs = 0U;
        componentState->simple.changeIndex = 0U;
        // TODO: Handle equation patterns
        return;
    }

    const SimplePattern *pattern = &component->pattern.data.simple;
    if (pattern->changeAt_count == 0U) {
        componentState->simple.elapsedMs = 0U;
        componentState->simple.changeIndex = 0U;
        return;
    }

    advanceSimplePattern(&componentState->simple, pattern, deltaMs);
}

void modeStateReset(ModeState *state, uint32_t initialMs) {
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->lastPatternUpdateMs = initialMs;
}

void modeStateAdvance(ModeState *state, const Mode *mode, uint32_t ms) {
    if (!state || !mode) {
        return;
    }

    uint32_t deltaMs = 0U;
    if (ms >= state->lastPatternUpdateMs) {
        deltaMs = ms - state->lastPatternUpdateMs;
    }
    state->lastPatternUpdateMs = ms;
    if (deltaMs == 0U) {
        return;
    }

    if (mode->has_front) {
        advanceComponentState(&state->front, &mode->front, deltaMs);
    }

    if (mode->has_case_comp) {
        advanceComponentState(&state->case_comp, &mode->case_comp, deltaMs);
    }

    if (mode->has_accel) {
        uint8_t triggerCount = mode->accel.triggers_count;
        if (triggerCount > MODE_ACCEL_TRIGGER_MAX) {
            triggerCount = MODE_ACCEL_TRIGGER_MAX;
        }

        for (uint8_t i = 0; i < triggerCount; i++) {
            if (mode->accel.triggers[i].has_front) {
                advanceComponentState(&state->accel[i].front, &mode->accel.triggers[i].front, deltaMs);
            }

            if (mode->accel.triggers[i].has_case_comp) {
                advanceComponentState(&state->accel[i].case_comp, &mode->accel.triggers[i].case_comp, deltaMs);
            }
        }
    }
}

bool modeStateGetSimpleOutput(const ModeComponentState *componentState, const ModeComponent *component, SimpleOutput *output) {
    if (!componentState || !component || !output) {
        return false;
    }

    if (component->pattern.type != PATTERN_TYPE_SIMPLE) {
        return false;
    }

    const SimplePattern *pattern = &component->pattern.data.simple;
    if (pattern->changeAt_count == 0U) {
        return false;
    }

    uint8_t index = componentState->simple.changeIndex;
    if (index >= pattern->changeAt_count) {
        index = 0U;
    }

    *output = pattern->changeAt[index].output;
    return true;
}
