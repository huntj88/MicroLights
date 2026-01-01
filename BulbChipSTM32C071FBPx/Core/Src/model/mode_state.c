#include "model/mode_state.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifdef UNIT_TEST
static uint32_t g_equationFreeCounter;

void modeStateTest_resetEquationFreeCounter(void) {
    g_equationFreeCounter = 0U;
}

uint32_t modeStateTest_getEquationFreeCounter(void) {
    return g_equationFreeCounter;
}

static inline void modeStateTest_noteEquationFree(void) {
    g_equationFreeCounter++;
}
#else
static inline void modeStateTest_noteEquationFree(void) {
}
#endif

enum { MODE_EQUATION_PATH_MAX = sizeof(((ModeEquationError *)0)->path) };

static void prependEquationContext(ModeEquationError *error, const char *segment, int32_t index) {
    if (!error || !error->hasError || !segment || segment[0] == '\0') {
        return;
    }

    char tmp[MODE_EQUATION_PATH_MAX * 2];
    if (error->path[0] == '\0') {
        if (index >= 0) {
            snprintf(tmp, sizeof(tmp), "%s[%ld]", segment, (long)index);
        } else {
            snprintf(tmp, sizeof(tmp), "%s", segment);
        }
    } else {
        if (index >= 0) {
            snprintf(tmp, sizeof(tmp), "%s[%ld].%s", segment, (long)index, error->path);
        } else {
            snprintf(tmp, sizeof(tmp), "%s.%s", segment, error->path);
        }
    }

    strncpy(error->path, tmp, sizeof(error->path) - 1U);
    error->path[sizeof(error->path) - 1U] = '\0';
}

static void freeEquationChannel(EquationChannelState *state) {
    if (!state) {
        return;
    }
    for (int i = 0; i < CHANNEL_CONFIG_SECTIONS_MAX; i++) {
        if (state->compiledExprs[i] != NULL) {
            modeStateTest_noteEquationFree();
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

static bool equationPatternAllowsLoop(const EquationPattern *pattern) {
    if (!pattern) {
        return false;
    }

    return pattern->red.loopAfterDuration && pattern->green.loopAfterDuration &&
           pattern->blue.loopAfterDuration;
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
    // Wrap elapsed time back into the pattern duration and jump to first change.
    while (elapsed >= duration) {
        elapsed -= duration;
        state->changeIndex = 0U;
    }

    state->elapsedMs = elapsed;

    // If time moved forward, walk the change index forward until the next change lies ahead.
    while ((state->changeIndex + 1U) < pattern->changeAtCount &&
           pattern->changeAt[state->changeIndex + 1U].ms <= state->elapsedMs) {
        state->changeIndex++;
    }

    // If time moved backward (e.g., wrap elapsed time by duration shrink loop), walk the change
    // index backward.
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
    bool allowLoop = (duration > 0U) && equationPatternAllowsLoop(pattern);
    bool looped = false;

    if (allowLoop) {
        uint32_t nextElapsed = state->elapsedMs + deltaMs;
        if (nextElapsed >= duration) {
            looped = true;
            state->elapsedMs = nextElapsed % duration;

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

static void captureEquationError(
    ModeEquationError *error, int errorPosition, const char *expression) {
    if (!error || error->hasError) {
        return;
    }

    error->hasError = true;

    error->path[0] = '\0';

    if (expression) {
        strncpy(error->equation, expression, sizeof(error->equation) - 1U);
        error->equation[sizeof(error->equation) - 1U] = '\0';
    }
    error->errorPosition = errorPosition;
}

static bool compileEquationChannel(
    EquationChannelState *state, const ChannelConfig *config, ModeEquationError *error) {
    assert(state != NULL);
    assert(config != NULL);
    if (!state || !config) {
        return false;
    }

    bool success = true;
    for (int i = 0; i < config->sectionsCount && i < CHANNEL_CONFIG_SECTIONS_MAX; i++) {
        int err;
        te_variable vars[] = {{"t", &state->t_var, 0, NULL}};

        char buffer[EQUATION_SECTION_EQUATION_MAX_LEN];
        strncpy(buffer, config->sections[i].equation, sizeof(buffer));

        for (size_t j = 0; j < sizeof(buffer) && buffer[j] != '\0'; j++) {
            buffer[j] = (char)tolower((unsigned char)buffer[j]);
        }

        state->compiledExprs[i] = te_compile(buffer, vars, 1, &err);
        if (!state->compiledExprs[i]) {
            success = false;
            captureEquationError(error, err, config->sections[i].equation);
            prependEquationContext(error, "sections", i);
        }
    }

    return success;
}

static bool compileEquationPattern(
    EquationPatternState *state, const EquationPattern *pattern, ModeEquationError *error) {
    assert(state != NULL);
    assert(pattern != NULL);
    if (!state || !pattern) {
        return false;
    }

    bool success = true;
    if (!compileEquationChannel(&state->red, &pattern->red, error)) {
        prependEquationContext(error, "red", -1);
        success = false;
    }
    if (!compileEquationChannel(&state->green, &pattern->green, error)) {
        prependEquationContext(error, "green", -1);
        success = false;
    }
    if (!compileEquationChannel(&state->blue, &pattern->blue, error)) {
        prependEquationContext(error, "blue", -1);
        success = false;
    }
    return success;
}

static bool compileComponentState(
    ModeComponentState *state, const ModeComponent *component, ModeEquationError *error) {
    assert(state != NULL);
    assert(component != NULL);
    if (!state || !component) {
        return false;
    }
    if (component->pattern.type == PATTERN_TYPE_EQUATION) {
        return compileEquationPattern(&state->equation, &component->pattern.data.equation, error);
    }
    return true;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static bool compileModeState(ModeState *state, const Mode *mode, ModeEquationError *error) {
    assert(state != NULL);
    assert(mode != NULL);
    if (!state || !mode) {
        return false;
    }

    bool success = true;
    if (mode->hasFront) {
        if (!compileComponentState(&state->front, &mode->front, error)) {
            prependEquationContext(error, "front", -1);
            success = false;
        }
    }
    if (mode->hasCaseComp) {
        if (!compileComponentState(&state->case_comp, &mode->caseComp, error)) {
            prependEquationContext(error, "caseComp", -1);
            success = false;
        }
    }
    if (mode->hasAccel) {
        for (int i = 0; i < mode->accel.triggersCount && i < MODE_ACCEL_TRIGGERS_MAX; i++) {
            if (mode->accel.triggers[i].hasFront) {
                if (!compileComponentState(
                        &state->accel[i].front, &mode->accel.triggers[i].front, error)) {
                    prependEquationContext(error, "front", -1);
                    prependEquationContext(error, "accel", i);
                    success = false;
                }
            }
            if (mode->accel.triggers[i].hasCaseComp) {
                if (!compileComponentState(
                        &state->accel[i].case_comp, &mode->accel.triggers[i].caseComp, error)) {
                    prependEquationContext(error, "caseComp", -1);
                    prependEquationContext(error, "accel", i);
                    success = false;
                }
            }
        }
    }

    return success;
}

bool modeStateInitialize(
    ModeState *state, const Mode *mode, uint32_t initialMs, ModeEquationError *error) {
    if (!state) {
        return false;
    }
    if (error) {
        memset(error, 0, sizeof(*error));
    }

    freeComponentState(&state->front);
    freeComponentState(&state->case_comp);
    for (int i = 0; i < MODE_ACCEL_TRIGGERS_MAX; i++) {
        freeComponentState(&state->accel[i].front);
        freeComponentState(&state->accel[i].case_comp);
    }

    memset(state, 0, sizeof(*state));
    state->lastPatternUpdateMs = initialMs;

    return compileModeState(state, mode, error);
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
        if (triggerCount > MODE_ACCEL_TRIGGERS_MAX) {
            triggerCount = MODE_ACCEL_TRIGGERS_MAX;
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
    if (state->currentSectionIndex >= CHANNEL_CONFIG_SECTIONS_MAX) {
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
