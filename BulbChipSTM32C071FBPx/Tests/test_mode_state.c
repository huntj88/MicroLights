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

void setUp(void) {
    memset(&mode, 0, sizeof(mode));
    memset(&state, 0, sizeof(state));
    memset(&output, 0, sizeof(output));
}

void tearDown(void) {
}

void test_ModeStateReset_SeedsInitialTime(void) {
    modeStateReset(&state, &mode, 1234U);
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

    modeStateReset(&state, &mode, 0U);
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

    modeStateReset(&state, &mode, 0U);
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

    modeStateReset(&state, &mode, 0);

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

    modeStateReset(&state, &mode, 0);

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

    modeStateReset(&state, &mode, 0U);
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

    modeStateReset(&state, &mode, 0);

    // Check if it compiled (compiledExprs should not be NULL)
    TEST_ASSERT_NOT_NULL(state.front.equation.red.compiledExprs[0]);

    // Advance and check output to ensure it evaluates
    modeStateAdvance(&state, &mode, 100);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ModeStateAdvance_CaseAndTriggersAdvance);
    RUN_TEST(test_ModeStateAdvance_FrontPatternAdvancesAndWraps);
    RUN_TEST(test_ModeStateAdvance_IgnoresNonMonotonicTime);
    RUN_TEST(test_ModeStateGetSimpleOutput_FalseWhenNoChanges);
    RUN_TEST(test_ModeStateReset_SeedsInitialTime);
    RUN_TEST(test_equation_case_insensitive);
    RUN_TEST(test_equation_multi_section);
    RUN_TEST(test_equation_pattern);
    return UNITY_END();
}
