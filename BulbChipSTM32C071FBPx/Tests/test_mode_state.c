#include <string.h>
#include "unity.h"

#include "model/mode_state.h"

static Mode mode;
static ModeState state;
static SimpleOutput output;

static void advance_to_ms(uint32_t targetMs) {
    uint32_t current = state.lastPatternUpdateMs;
    if (targetMs <= current) {
        modeStateAdvance(&state, &mode, targetMs);
        return;
    }
    uint32_t next = (current / 10U + 1U) * 10U;
    while (next <= targetMs) {
        modeStateAdvance(&state, &mode, next);
        next += 10U;
    }
}

static void init_simple_pattern(SimplePattern *pattern, uint32_t duration) {
    memset(pattern, 0, sizeof(*pattern));
    pattern->duration = duration;
}

static void add_bulb_change(
    SimplePattern *pattern, uint8_t index, uint32_t ms, BulbSimpleOutput level) {
    pattern->changeAt[index].ms = ms;
    pattern->changeAt[index].output.type = BULB;
    pattern->changeAt[index].output.data.bulb = level;
    if (index + 1U > pattern->changeAtCount) {
        pattern->changeAtCount = index + 1U;
    }
}

static void add_rgb_change(
    SimplePattern *pattern, uint8_t index, uint32_t ms, uint8_t r, uint8_t g, uint8_t b) {
    pattern->changeAt[index].ms = ms;
    pattern->changeAt[index].output.type = RGB;
    pattern->changeAt[index].output.data.rgb.r = r;
    pattern->changeAt[index].output.data.rgb.g = g;
    pattern->changeAt[index].output.data.rgb.b = b;
    if (index + 1U > pattern->changeAtCount) {
        pattern->changeAtCount = index + 1U;
    }
}

static void init_equation_channel(ChannelConfig *channel, const char *equation, uint32_t duration) {
    memset(channel, 0, sizeof(*channel));
    channel->sectionsCount = 1;
    strncpy(channel->sections[0].equation, equation, sizeof(channel->sections[0].equation) - 1U);
    channel->sections[0].equation[sizeof(channel->sections[0].equation) - 1U] = '\0';
    channel->sections[0].duration = duration;
    channel->loopAfterDuration = true;
}

void setUp(void) {
    memset(&mode, 0, sizeof(mode));
    memset(&state, 0, sizeof(state));
    memset(&output, 0, sizeof(output));
    
    // used for validating no memory leaks in equation patterns
    modeStateTest_resetEquationFreeCounter();
}

void tearDown(void) {
}

void test_ModeStateInitialize_SeedsInitialTime(void) {
    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 1234U, NULL));
    TEST_ASSERT_EQUAL_UINT32(1234U, state.lastPatternUpdateMs);
    TEST_ASSERT_EQUAL_UINT8(0, state.front.simple.changeIndex);
}

void test_ModeStateAdvance_FrontPatternAdvancesAndWraps(void) {
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.front.pattern.data.simple, 100U);
    add_bulb_change(&mode.front.pattern.data.simple, 0, 0U, high);
    add_bulb_change(&mode.front.pattern.data.simple, 1, 50U, low);
    TEST_ASSERT_EQUAL_UINT8(2, mode.front.pattern.data.simple.changeAtCount);
    TEST_ASSERT_EQUAL_UINT32(0U, mode.front.pattern.data.simple.changeAt[0].ms);
    TEST_ASSERT_EQUAL_UINT32(50U, mode.front.pattern.data.simple.changeAt[1].ms);

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0U, NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, state.front.simple.elapsedMs);

    advance_to_ms(10U);
    TEST_ASSERT_EQUAL_UINT32(10U, state.front.simple.elapsedMs);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(high, output.data.bulb);

    advance_to_ms(60U);
    TEST_ASSERT_EQUAL_UINT32(60U, state.front.simple.elapsedMs);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(low, output.data.bulb);

    advance_to_ms(210U);
    TEST_ASSERT_EQUAL_UINT32(10U, state.front.simple.elapsedMs);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(high, output.data.bulb);
}

void test_ModeStateAdvance_CaseAndTriggersAdvance(void) {
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.front.pattern.data.simple, 80U);
    add_bulb_change(&mode.front.pattern.data.simple, 0, 0U, high);
    add_bulb_change(&mode.front.pattern.data.simple, 1, 40U, low);

    mode.hasCaseComp = true;
    mode.caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.caseComp.pattern.data.simple, 60U);
    add_rgb_change(&mode.caseComp.pattern.data.simple, 0, 0U, 0, 0, 255);
    add_rgb_change(&mode.caseComp.pattern.data.simple, 1, 30U, 0, 255, 0);

    mode.hasAccel = true;
    mode.accel.triggersCount = 1;
    mode.accel.triggers[0].hasFront = true;
    mode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.accel.triggers[0].front.pattern.data.simple, 20U);
    add_bulb_change(&mode.accel.triggers[0].front.pattern.data.simple, 0, 0U, low);
    add_bulb_change(&mode.accel.triggers[0].front.pattern.data.simple, 1, 10U, high);

    mode.accel.triggers[0].hasCaseComp = true;
    mode.accel.triggers[0].caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.accel.triggers[0].caseComp.pattern.data.simple, 20U);
    add_rgb_change(&mode.accel.triggers[0].caseComp.pattern.data.simple, 0, 0U, 255, 0, 0);
    add_rgb_change(&mode.accel.triggers[0].caseComp.pattern.data.simple, 1, 10U, 255, 255, 0);

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0U, NULL));
    advance_to_ms(10U);
    TEST_ASSERT_TRUE(
        modeStateGetSimpleOutput(&state.accel[0].front, &mode.accel.triggers[0].front, &output));
    TEST_ASSERT_EQUAL_UINT8(high, output.data.bulb);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(
        &state.accel[0].case_comp, &mode.accel.triggers[0].caseComp, &output));
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.g);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.b);

    advance_to_ms(40U);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(low, output.data.bulb);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.case_comp, &mode.caseComp, &output));
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.g);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.b);
    TEST_ASSERT_TRUE(
        modeStateGetSimpleOutput(&state.accel[0].front, &mode.accel.triggers[0].front, &output));
    TEST_ASSERT_EQUAL_UINT8(low, output.data.bulb);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(
        &state.accel[0].case_comp, &mode.accel.triggers[0].caseComp, &output));
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.g);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.b);
}

void test_equation_pattern(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->duration = 2000;

    // Red: t * 100
    eq->red.sectionsCount = 1;
    strcpy(eq->red.sections[0].equation, "t * 100");
    eq->red.sections[0].duration = 2000;
    eq->red.loopAfterDuration = true;

    // Green: 255 - t * 100
    eq->green.sectionsCount = 1;
    strcpy(eq->green.sections[0].equation, "255 - t * 100");
    eq->green.sections[0].duration = 2000;
    eq->green.loopAfterDuration = true;

    // Blue: 0
    eq->blue.sectionsCount = 0;

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));

    // t = 0
    modeStateAdvance(&state, &mode, 0);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL(RGB, output.type);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.g);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.b);

    // t = 1.0s
    modeStateAdvance(&state, &mode, 1000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(100, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(155, output.data.rgb.g);

    // t = 2.0s (loop)
    modeStateAdvance(&state, &mode, 2000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.g);
}

void test_equation_pattern_respects_channel_loop_flags(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->duration = 1000;

    // Red disables looping
    eq->red.sectionsCount = 1;
    strcpy(eq->red.sections[0].equation, "t * 100");
    eq->red.sections[0].duration = 1000;
    eq->red.loopAfterDuration = false;

    // Green continues looping to prove per-channel loops still work
    eq->green.sectionsCount = 1;
    strcpy(eq->green.sections[0].equation, "t * 100");
    eq->green.sections[0].duration = 1000;
    eq->green.loopAfterDuration = true;

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));

    modeStateAdvance(&state, &mode, 0);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.g);

    // 500ms -> both channels report 50
    modeStateAdvance(&state, &mode, 500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.g);

    // 1500ms -> pattern duration exceeded, red keeps growing, green loops
    modeStateAdvance(&state, &mode, 1500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(150, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.g);

    // 2500ms -> red continues, green still loops
    modeStateAdvance(&state, &mode, 2500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(250, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.g);
}

void test_equation_multi_section(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->duration = 0;  // Infinite

    // Red: 0-1s: 100, 1-2s: 200
    eq->red.sectionsCount = 2;
    strcpy(eq->red.sections[0].equation, "100");
    eq->red.sections[0].duration = 1000;
    strcpy(eq->red.sections[1].equation, "200");
    eq->red.sections[1].duration = 1000;
    eq->red.loopAfterDuration = true;

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));

    // t = 0
    modeStateAdvance(&state, &mode, 0);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(100, output.data.rgb.r);

    // t = 500ms
    modeStateAdvance(&state, &mode, 500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(100, output.data.rgb.r);

    // t = 1000ms (switch to section 1)
    modeStateAdvance(&state, &mode, 1000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(200, output.data.rgb.r);

    // t = 1500ms
    modeStateAdvance(&state, &mode, 1500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(200, output.data.rgb.r);

    // t = 2000ms (loop back to section 0)
    modeStateAdvance(&state, &mode, 2000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(100, output.data.rgb.r);
}

void test_ModeStateAdvance_IgnoresNonMonotonicTime(void) {
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.front.pattern.data.simple, 100U);
    add_bulb_change(&mode.front.pattern.data.simple, 0, 0U, high);
    add_bulb_change(&mode.front.pattern.data.simple, 1, 50U, low);

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0U, NULL));
    modeStateAdvance(&state, &mode, 60U);
    TEST_ASSERT_EQUAL_UINT8(1, state.front.simple.changeIndex);

    modeStateAdvance(&state, &mode, 20U);
    TEST_ASSERT_EQUAL_UINT8(1, state.front.simple.changeIndex);
}

void test_ModeStateGetSimpleOutput_FalseWhenNoChanges(void) {
    ModeComponent component = {0};
    component.pattern.type = PATTERN_TYPE_SIMPLE;
    component.pattern.data.simple.duration = 100U;
    component.pattern.data.simple.changeAtCount = 0U;

    TEST_ASSERT_FALSE(modeStateGetSimpleOutput(&state.front, &component, &output));

    // component.pattern.type = PATTERN_TYPE_EQUATION;
    // component.pattern.data.simple.changeAtCount = 1U;
    // TEST_ASSERT_FALSE(modeStateGetSimpleOutput(&state.front, &component, &output));
}

void test_equation_case_insensitive(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->duration = 1000;

    // Red: ABS(SIN(t * 8 + PI / 3 * 2)) * 255
    eq->red.sectionsCount = 1;
    strcpy(eq->red.sections[0].equation, "ABS(SIN(t * 8 + PI / 3 * 2)) * 255");
    eq->red.sections[0].duration = 1000;

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));

    // Check if it compiled (compiledExprs should not be NULL)
    TEST_ASSERT_NOT_NULL(state.front.equation.red.compiledExprs[0]);

    // Advance and check output to ensure it evaluates
    modeStateAdvance(&state, &mode, 100);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
}

void test_ModeStateInitialize_FailsOnInvalidEquation(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->red.sectionsCount = 1;
    strcpy(eq->red.sections[0].equation, "bad +");
    eq->red.sections[0].duration = 1000;

    ModeEquationError error = {0};
    bool ok = modeStateInitialize(&state, &mode, 0, &error);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(error.hasError);
    TEST_ASSERT_EQUAL_STRING("front.red.sections[0]", error.path);
    TEST_ASSERT_EQUAL_STRING("bad +", error.equation);
}

void test_ModeStateInitialize_ReportsAccelEquationError(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasAccel = true;
    mode.accel.triggersCount = 1;
    mode.accel.triggers[0].hasCaseComp = true;
    mode.accel.triggers[0].caseComp.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.accel.triggers[0].caseComp.pattern.data.equation;
    eq->green.sectionsCount = 1;
    strcpy(eq->green.sections[0].equation, "??invalid");
    eq->green.sections[0].duration = 500;

    ModeEquationError error = {0};
    bool ok = modeStateInitialize(&state, &mode, 0, &error);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(error.hasError);
    TEST_ASSERT_EQUAL_STRING("accel[0].caseComp.green.sections[0]", error.path);
    TEST_ASSERT_EQUAL_STRING("??invalid", error.equation);
}

void test_ModeStateInitialize_FreesFrontAndCaseEquationsOnReinit(void) {
    TEST_ASSERT_TRUE(modeStateTest_getEquationFreeCounter() == 0U);
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *front = &mode.front.pattern.data.equation;
    init_equation_channel(&front->red, "10", 1000);
    init_equation_channel(&front->green, "20", 1000);
    init_equation_channel(&front->blue, "30", 1000);

    mode.hasCaseComp = true;
    mode.caseComp.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *caseComp = &mode.caseComp.pattern.data.equation;
    init_equation_channel(&caseComp->red, "40", 1000);
    init_equation_channel(&caseComp->green, "50", 1000);
    init_equation_channel(&caseComp->blue, "60", 1000);

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));
    // 0 equations freed on first init
    TEST_ASSERT_EQUAL_UINT32(0U, modeStateTest_getEquationFreeCounter());

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));
    TEST_ASSERT_EQUAL_UINT32(6U, modeStateTest_getEquationFreeCounter());
}

void test_ModeStateInitialize_FreesAccelEquationsOnReinit(void) {
    TEST_ASSERT_TRUE(modeStateTest_getEquationFreeCounter() == 0U);
    memset(&mode, 0, sizeof(mode));
    mode.hasAccel = true;
    mode.accel.triggersCount = 2;

    for (uint8_t i = 0; i < mode.accel.triggersCount; i++) {
        ModeAccelTrigger *trigger = &mode.accel.triggers[i];
        trigger->hasFront = true;
        trigger->front.pattern.type = PATTERN_TYPE_EQUATION;
        EquationPattern *frontEq = &trigger->front.pattern.data.equation;
        init_equation_channel(&frontEq->red, "t", 500);
        init_equation_channel(&frontEq->green, "t * 2", 500);
        init_equation_channel(&frontEq->blue, "t * 3", 500);

        trigger->hasCaseComp = true;
        trigger->caseComp.pattern.type = PATTERN_TYPE_EQUATION;
        EquationPattern *caseEq = &trigger->caseComp.pattern.data.equation;
        init_equation_channel(&caseEq->red, "t * 4", 500);
        init_equation_channel(&caseEq->green, "t * 5", 500);
        init_equation_channel(&caseEq->blue, "t * 6", 500);
    }

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));
    // 0 equations freed on first init
    TEST_ASSERT_EQUAL_UINT32(0U, modeStateTest_getEquationFreeCounter());

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));
    TEST_ASSERT_EQUAL_UINT32(12U, modeStateTest_getEquationFreeCounter());
}

void test_equation_loopAfterDuration_false_continues_indefinitely(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->duration = 0;  // Infinite pattern duration

    // Red: t * 10 (grows with time)
    // With loopAfterDuration = false, t should continue past section duration
    eq->red.sectionsCount = 1;
    strcpy(eq->red.sections[0].equation, "t * 10");
    eq->red.sections[0].duration = 1000;  // Section duration 1s
    eq->red.loopAfterDuration = false;

    // Green: constant 50
    eq->green.sectionsCount = 1;
    strcpy(eq->green.sections[0].equation, "50");
    eq->green.sections[0].duration = 1000;
    eq->green.loopAfterDuration = false;

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));

    // t = 0
    modeStateAdvance(&state, &mode, 0);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.r);

    // t = 1.0s (at section duration boundary)
    modeStateAdvance(&state, &mode, 1000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(10, output.data.rgb.r);  // t=1.0 => 10

    // t = 2.0s (past section duration - t should continue growing)
    modeStateAdvance(&state, &mode, 2000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(20, output.data.rgb.r);  // t=2.0 => 20

    // t = 5.0s (well past section duration)
    modeStateAdvance(&state, &mode, 5000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);  // t=5.0 => 50

    // t = 25.5s (t grows indefinitely)
    modeStateAdvance(&state, &mode, 25500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.r);  // t=25.5 => 255 (clamped)
}

void test_equation_loopAfterDuration_false_multi_section_stays_on_last(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->duration = 0;

    // Red: Section 0: 50 for 1s, Section 1: t * 100 (continues indefinitely)
    eq->red.sectionsCount = 2;
    strcpy(eq->red.sections[0].equation, "50");
    eq->red.sections[0].duration = 1000;
    strcpy(eq->red.sections[1].equation, "t * 100");
    eq->red.sections[1].duration = 1000;  // This duration should be ignored
    eq->red.loopAfterDuration = false;

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));

    // t = 0 (section 0)
    modeStateAdvance(&state, &mode, 0);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);

    // t = 500ms (still section 0)
    modeStateAdvance(&state, &mode, 500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);

    // t = 1000ms (transition to section 1, section t resets to 0)
    modeStateAdvance(&state, &mode, 1000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.r);  // t=0 in new section

    // t = 1500ms (section 1, t=0.5s)
    modeStateAdvance(&state, &mode, 1500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);  // t=0.5 => 50

    // t = 2000ms (still section 1, past its duration, t=1.0s)
    modeStateAdvance(&state, &mode, 2000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(100, output.data.rgb.r);  // t=1.0 => 100

    // t = 3000ms (still section 1, t=2.0s - continues indefinitely)
    modeStateAdvance(&state, &mode, 3000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(200, output.data.rgb.r);  // t=2.0 => 200
}

void test_equation_loopAfterDuration_true_loops_back(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->duration = 0;

    // Red: t * 100 with looping enabled
    eq->red.sectionsCount = 1;
    strcpy(eq->red.sections[0].equation, "t * 100");
    eq->red.sections[0].duration = 1000;
    eq->red.loopAfterDuration = true;

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));

    // t = 500ms
    modeStateAdvance(&state, &mode, 500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);  // t=0.5 => 50

    // t = 1000ms (loops back, t resets)
    modeStateAdvance(&state, &mode, 1000);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.r);  // t=0 after loop

    // t = 1500ms
    modeStateAdvance(&state, &mode, 1500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);  // t=0.5 => 50
}

void test_equation_loopAfterDuration_mixed_channels(void) {
    memset(&mode, 0, sizeof(mode));
    mode.hasFront = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *eq = &mode.front.pattern.data.equation;
    eq->duration = 0;

    // Red: loops (t * 100)
    eq->red.sectionsCount = 1;
    strcpy(eq->red.sections[0].equation, "t * 100");
    eq->red.sections[0].duration = 1000;
    eq->red.loopAfterDuration = true;

    // Green: does not loop (t * 50)
    eq->green.sectionsCount = 1;
    strcpy(eq->green.sections[0].equation, "t * 50");
    eq->green.sections[0].duration = 1000;
    eq->green.loopAfterDuration = false;

    TEST_ASSERT_TRUE(modeStateInitialize(&state, &mode, 0, NULL));

    // t = 500ms
    modeStateAdvance(&state, &mode, 500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);  // t=0.5 => 50
    TEST_ASSERT_EQUAL_UINT8(25, output.data.rgb.g);  // t=0.5 => 25

    // t = 1500ms - red loops back, green continues
    modeStateAdvance(&state, &mode, 1500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);  // t=0.5 (looped) => 50
    TEST_ASSERT_EQUAL_UINT8(75, output.data.rgb.g);  // t=1.5 => 75

    // t = 2500ms - red loops again, green continues growing
    modeStateAdvance(&state, &mode, 2500);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(50, output.data.rgb.r);   // t=0.5 (looped) => 50
    TEST_ASSERT_EQUAL_UINT8(125, output.data.rgb.g);  // t=2.5 => 125
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ModeStateAdvance_CaseAndTriggersAdvance);
    RUN_TEST(test_ModeStateAdvance_FrontPatternAdvancesAndWraps);
    RUN_TEST(test_ModeStateAdvance_IgnoresNonMonotonicTime);
    RUN_TEST(test_ModeStateGetSimpleOutput_FalseWhenNoChanges);
    RUN_TEST(test_ModeStateInitialize_FailsOnInvalidEquation);
    RUN_TEST(test_ModeStateInitialize_FreesAccelEquationsOnReinit);
    RUN_TEST(test_ModeStateInitialize_FreesFrontAndCaseEquationsOnReinit);
    RUN_TEST(test_ModeStateInitialize_ReportsAccelEquationError);
    RUN_TEST(test_ModeStateInitialize_SeedsInitialTime);
    RUN_TEST(test_equation_case_insensitive);
    RUN_TEST(test_equation_loopAfterDuration_false_continues_indefinitely);
    RUN_TEST(test_equation_loopAfterDuration_false_multi_section_stays_on_last);
    RUN_TEST(test_equation_loopAfterDuration_mixed_channels);
    RUN_TEST(test_equation_loopAfterDuration_true_loops_back);
    RUN_TEST(test_equation_multi_section);
    RUN_TEST(test_equation_pattern);
    RUN_TEST(test_equation_pattern_respects_channel_loop_flags);
    return UNITY_END();
}
