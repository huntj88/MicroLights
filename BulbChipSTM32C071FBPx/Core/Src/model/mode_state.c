#include "model/mode_state.h"

#include <ctype.h>
#include <string.h>

static void freeEquationChannel(EquationChannelState *state) {
    for (int i = 0; i < 3; i++) {
        if (state->compiledExprs[i]) {
            te_free(state->compiledExprs[i]);
            state->compiledExprs[i] = NULL;
        }
    }
}

static void freeEquationPattern(EquationPatternState *state) {
    freeEquationChannel(&state->red);
    freeEquationChannel(&state->green);
    freeEquationChannel(&state->blue);
}

static void freeComponentState(ModeComponentState *state) {
    freeEquationPattern(&state->equation);
}

static void advanceSimplePattern(
    SimplePatternState *state, const SimplePattern *pattern, uint32_t deltaMs) {
    if (!state || !pattern || pattern->changeAtCount == 0U || deltaMs == 0U) {
        if (state && pattern && pattern->changeAtCount > 0U &&
            state->changeIndex >= pattern->changeAtCount) {
            state->changeIndex = pattern->changeAtCount - 1U;
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

    while ((state->changeIndex + 1U) < pattern->changeAtCount &&
           pattern->changeAt[state->changeIndex + 1U].ms <= state->elapsedMs) {
        state->changeIndex++;
    }

    while (state->changeIndex > 0U && pattern->changeAt[state->changeIndex].ms > state->elapsedMs) {
        state->changeIndex--;
    }
}

static void advanceEquationChannel(
    EquationChannelState *state, const ChannelConfig *config, uint32_t deltaMs) {
    if (!state || !config || config->sectionsCount == 0) {
        return;
    }

    state->sectionElapsedMs += deltaMs;

    // Only check for section transitions if not on the last section,
    // or if looping is enabled. When on last section with looping disabled,
    // let it continue indefinitely.
    bool isLastSection = (state->currentSectionIndex >= config->sectionsCount - 1);
    bool shouldCheckDuration = !isLastSection || config->loopAfterDuration;

    if (shouldCheckDuration && state->currentSectionIndex < config->sectionsCount) {
        const EquationSection *section = &config->sections[state->currentSectionIndex];
        // Check if we need to move to next section
        if (state->sectionElapsedMs >= section->duration) {
            state->sectionElapsedMs -= section->duration;

            state->currentSectionIndex++;
            if (state->currentSectionIndex >= config->sectionsCount) {
                state->currentSectionIndex = 0;
            }
        }
    }

    // Update t_var (in seconds)
    state->t_var = (float)state->sectionElapsedMs / 1000.0F;
}

static void advanceEquationPattern(
    EquationPatternState *state, const EquationPattern *pattern, uint32_t deltaMs) {
    if (!state || !pattern) {
        return;
    }

    uint32_t duration = pattern->duration;
    bool looped = false;

    if (duration > 0) {
        uint32_t nextElapsed = state->elapsedMs + deltaMs;
        if (nextElapsed >= duration) {
            looped = true;
            state->elapsedMs = nextElapsed % duration;

            // Reset channels
            // freeEquationPattern(state); // No longer needed as we pre-compile

            state->red.currentSectionIndex = 0;
            state->red.sectionElapsedMs = 0;

            state->green.currentSectionIndex = 0;
            state->green.sectionElapsedMs = 0;

            state->blue.currentSectionIndex = 0;
            state->blue.sectionElapsedMs = 0;

            // Advance by the remainder
            advanceEquationChannel(&state->red, &pattern->red, state->elapsedMs);
            advanceEquationChannel(&state->green, &pattern->green, state->elapsedMs);
            advanceEquationChannel(&state->blue, &pattern->blue, state->elapsedMs);
        } else {
            state->elapsedMs = nextElapsed;
        }
    } else {
        state->elapsedMs += deltaMs;
    }

    if (!looped) {
        advanceEquationChannel(&state->red, &pattern->red, deltaMs);
        advanceEquationChannel(&state->green, &pattern->green, deltaMs);
        advanceEquationChannel(&state->blue, &pattern->blue, deltaMs);
    }
}

static void advanceComponentState(
    ModeComponentState *componentState, const ModeComponent *component, uint32_t deltaMs) {
    if (!componentState || !component) {
        return;
    }

    if (component->pattern.type == PATTERN_TYPE_SIMPLE) {
        const SimplePattern *pattern = &component->pattern.data.simple;
        if (pattern->changeAtCount == 0U) {
            componentState->simple.elapsedMs = 0U;
            componentState->simple.changeIndex = 0U;
            return;
        }
        advanceSimplePattern(&componentState->simple, pattern, deltaMs);
    } else if (component->pattern.type == PATTERN_TYPE_EQUATION) {
        advanceEquationPattern(
            &componentState->equation, &component->pattern.data.equation, deltaMs);
    }
}

static void compileEquationChannel(EquationChannelState *state, const ChannelConfig *config) {
    if (!state || !config) {
        return;
    }

    for (int i = 0; i < config->sectionsCount && i < 3; i++) {
        int err;
        te_variable vars[] = {{"t", &state->t_var, 0, NULL}};

        unsigned char buffer[64];
        strncpy(buffer, config->sections[i].equation, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        for (int j = 0; buffer[j]; j++) {
            buffer[j] = tolower(buffer[j]);
        }

        state->compiledExprs[i] = te_compile(buffer, vars, 1, &err);
    }
}

static void compileEquationPattern(EquationPatternState *state, const EquationPattern *pattern) {
    if (!state || !pattern) {
        return;
    }
    compileEquationChannel(&state->red, &pattern->red);
    compileEquationChannel(&state->green, &pattern->green);
    compileEquationChannel(&state->blue, &pattern->blue);
}

static void compileComponentState(ModeComponentState *state, const ModeComponent *component) {
    if (!state || !component) {
        return;
    }
    if (component->pattern.type == PATTERN_TYPE_EQUATION) {
        compileEquationPattern(&state->equation, &component->pattern.data.equation);
    }
}

void modeStateReset(ModeState *state, const Mode *mode, uint32_t initialMs) {
    if (!state) {
        return;
    }

    freeComponentState(&state->front);
    freeComponentState(&state->case_comp);
    for (int i = 0; i < MODE_ACCEL_TRIGGER_MAX; i++) {
        freeComponentState(&state->accel[i].front);
        freeComponentState(&state->accel[i].case_comp);
    }

    memset(state, 0, sizeof(*state));
    state->lastPatternUpdateMs = initialMs;

    if (mode) {
        if (mode->hasFront) {
            compileComponentState(&state->front, &mode->front);
        }
        if (mode->hasCaseComp) {
            compileComponentState(&state->case_comp, &mode->caseComp);
        }
        if (mode->hasAccel) {
            for (int i = 0; i < mode->accel.triggersCount && i < MODE_ACCEL_TRIGGER_MAX; i++) {
                if (mode->accel.triggers[i].hasFront) {
                    compileComponentState(&state->accel[i].front, &mode->accel.triggers[i].front);
                }
                if (mode->accel.triggers[i].hasCaseComp) {
                    compileComponentState(
                        &state->accel[i].case_comp, &mode->accel.triggers[i].caseComp);
                }
            }
        }
    }
}

void modeStateAdvance(ModeState *state, const Mode *mode, uint32_t milliseconds) {
    if (!state || !mode) {
        return;
    }

    uint32_t deltaMs = 0U;
    if (milliseconds >= state->lastPatternUpdateMs) {
        deltaMs = milliseconds - state->lastPatternUpdateMs;
    }
    state->lastPatternUpdateMs = milliseconds;
    if (deltaMs == 0U) {
        return;
    }

    if (mode->hasFront) {
        advanceComponentState(&state->front, &mode->front, deltaMs);
    }

    if (mode->hasCaseComp) {
        advanceComponentState(&state->case_comp, &mode->caseComp, deltaMs);
    }

    if (mode->hasAccel) {
        uint8_t triggerCount = mode->accel.triggersCount;
        if (triggerCount > MODE_ACCEL_TRIGGER_MAX) {
            triggerCount = MODE_ACCEL_TRIGGER_MAX;
        }

        for (uint8_t i = 0; i < triggerCount; i++) {
            if (mode->accel.triggers[i].hasFront) {
                advanceComponentState(
                    &state->accel[i].front, &mode->accel.triggers[i].front, deltaMs);
            }

            if (mode->accel.triggers[i].hasCaseComp) {
                advanceComponentState(
                    &state->accel[i].case_comp, &mode->accel.triggers[i].caseComp, deltaMs);
            }
        }
    }
}

static uint8_t evalChannel(const EquationChannelState *state) {
    if (state->currentSectionIndex >= 3) {
        return 0;
    }
    te_expr *expr = state->compiledExprs[state->currentSectionIndex];
    if (!expr) {
        return 0;
    }

    float val = te_eval(expr);
    if (val < 0) {
        val = 0;
    }
    if (val > 255) {
        val = 255;
    }
    return (uint8_t)val;
}

bool modeStateGetSimpleOutput(
    const ModeComponentState *componentState,
    const ModeComponent *component,
    SimpleOutput *output) {
    if (!componentState || !component || !output) {
        return false;
    }

    if (component->pattern.type == PATTERN_TYPE_SIMPLE) {
        const SimplePattern *pattern = &component->pattern.data.simple;
        if (pattern->changeAtCount == 0U) {
            return false;
        }

        uint8_t index = componentState->simple.changeIndex;
        if (index >= pattern->changeAtCount) {
            index = 0U;
        }

        *output = pattern->changeAt[index].output;
        return true;
    }
    
    if (component->pattern.type == PATTERN_TYPE_EQUATION) {
        const EquationPatternState *state = &componentState->equation;
        output->type = RGB;
        output->data.rgb.r = evalChannel(&state->red);
        output->data.rgb.g = evalChannel(&state->green);
        output->data.rgb.b = evalChannel(&state->blue);
        return true;
    }

    return false;
}
