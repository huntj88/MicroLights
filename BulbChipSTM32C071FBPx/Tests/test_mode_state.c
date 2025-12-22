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
    if (index + 1U > pattern->changeAt_count) {
        pattern->changeAt_count = index + 1U;
    }
}

static void add_rgb_change(
    SimplePattern *pattern, uint8_t index, uint32_t ms, uint8_t r, uint8_t g, uint8_t b) {
    pattern->changeAt[index].ms = ms;
    pattern->changeAt[index].output.type = RGB;
    pattern->changeAt[index].output.data.rgb.r = r;
    pattern->changeAt[index].output.data.rgb.g = g;
    pattern->changeAt[index].output.data.rgb.b = b;
    if (index + 1U > pattern->changeAt_count) {
        pattern->changeAt_count = index + 1U;
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
    modeStateReset(&state, 1234U);
    TEST_ASSERT_EQUAL_UINT32(1234U, state.lastPatternUpdateMs);
    TEST_ASSERT_EQUAL_UINT8(0, state.front.simple.changeIndex);
}

void test_ModeStateAdvance_FrontPatternAdvancesAndWraps(void) {
    mode.has_front = true;
    mode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.front.pattern.data.simple, 100U);
    add_bulb_change(&mode.front.pattern.data.simple, 0, 0U, high);
    add_bulb_change(&mode.front.pattern.data.simple, 1, 50U, low);
    TEST_ASSERT_EQUAL_UINT8(2, mode.front.pattern.data.simple.changeAt_count);
    TEST_ASSERT_EQUAL_UINT32(0U, mode.front.pattern.data.simple.changeAt[0].ms);
    TEST_ASSERT_EQUAL_UINT32(50U, mode.front.pattern.data.simple.changeAt[1].ms);

    modeStateReset(&state, 0U);
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
    mode.has_front = true;
    mode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.front.pattern.data.simple, 80U);
    add_bulb_change(&mode.front.pattern.data.simple, 0, 0U, high);
    add_bulb_change(&mode.front.pattern.data.simple, 1, 40U, low);

    mode.has_case_comp = true;
    mode.case_comp.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.case_comp.pattern.data.simple, 60U);
    add_rgb_change(&mode.case_comp.pattern.data.simple, 0, 0U, 0, 0, 255);
    add_rgb_change(&mode.case_comp.pattern.data.simple, 1, 30U, 0, 255, 0);

    mode.has_accel = true;
    mode.accel.triggers_count = 1;
    mode.accel.triggers[0].has_front = true;
    mode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.accel.triggers[0].front.pattern.data.simple, 20U);
    add_bulb_change(&mode.accel.triggers[0].front.pattern.data.simple, 0, 0U, low);
    add_bulb_change(&mode.accel.triggers[0].front.pattern.data.simple, 1, 10U, high);

    mode.accel.triggers[0].has_case_comp = true;
    mode.accel.triggers[0].case_comp.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.accel.triggers[0].case_comp.pattern.data.simple, 20U);
    add_rgb_change(&mode.accel.triggers[0].case_comp.pattern.data.simple, 0, 0U, 255, 0, 0);
    add_rgb_change(&mode.accel.triggers[0].case_comp.pattern.data.simple, 1, 10U, 255, 255, 0);

    modeStateReset(&state, 0U);
    advance_to_ms(10U);
    TEST_ASSERT_TRUE(
        modeStateGetSimpleOutput(&state.accel[0].front, &mode.accel.triggers[0].front, &output));
    TEST_ASSERT_EQUAL_UINT8(high, output.data.bulb);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(
        &state.accel[0].case_comp, &mode.accel.triggers[0].case_comp, &output));
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.g);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.b);

    advance_to_ms(40U);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.front, &mode.front, &output));
    TEST_ASSERT_EQUAL_UINT8(low, output.data.bulb);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(&state.case_comp, &mode.case_comp, &output));
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.g);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.b);
    TEST_ASSERT_TRUE(
        modeStateGetSimpleOutput(&state.accel[0].front, &mode.accel.triggers[0].front, &output));
    TEST_ASSERT_EQUAL_UINT8(low, output.data.bulb);
    TEST_ASSERT_TRUE(modeStateGetSimpleOutput(
        &state.accel[0].case_comp, &mode.accel.triggers[0].case_comp, &output));
    TEST_ASSERT_EQUAL_UINT8(255, output.data.rgb.r);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.g);
    TEST_ASSERT_EQUAL_UINT8(0, output.data.rgb.b);
}

void test_ModeStateAdvance_IgnoresEquationPatterns(void) {
    mode.has_front = true;
    mode.front.pattern.type = PATTERN_TYPE_EQUATION;
    state.front.simple.changeIndex = 5U;
    state.front.simple.elapsedMs = 42U;

    modeStateReset(&state, 0U);
    state.front.simple.changeIndex = 5U;
    state.front.simple.elapsedMs = 42U;

    modeStateAdvance(&state, &mode, 100U);
    TEST_ASSERT_EQUAL_UINT8(0, state.front.simple.changeIndex);
    TEST_ASSERT_EQUAL_UINT32(0U, state.front.simple.elapsedMs);
}

void test_ModeStateAdvance_IgnoresNonMonotonicTime(void) {
    mode.has_front = true;
    mode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    init_simple_pattern(&mode.front.pattern.data.simple, 100U);
    add_bulb_change(&mode.front.pattern.data.simple, 0, 0U, high);
    add_bulb_change(&mode.front.pattern.data.simple, 1, 50U, low);

    modeStateReset(&state, 0U);
    modeStateAdvance(&state, &mode, 60U);
    TEST_ASSERT_EQUAL_UINT8(1, state.front.simple.changeIndex);

    modeStateAdvance(&state, &mode, 20U);
    TEST_ASSERT_EQUAL_UINT8(1, state.front.simple.changeIndex);
}

void test_ModeStateGetSimpleOutput_FalseWhenNoChanges(void) {
    ModeComponent component = {0};
    component.pattern.type = PATTERN_TYPE_SIMPLE;
    component.pattern.data.simple.duration = 100U;
    component.pattern.data.simple.changeAt_count = 0U;

    TEST_ASSERT_FALSE(modeStateGetSimpleOutput(&state.front, &component, &output));

    component.pattern.type = PATTERN_TYPE_EQUATION;
    component.pattern.data.simple.changeAt_count = 1U;
    TEST_ASSERT_FALSE(modeStateGetSimpleOutput(&state.front, &component, &output));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ModeStateReset_SeedsInitialTime);
    RUN_TEST(test_ModeStateAdvance_FrontPatternAdvancesAndWraps);
    RUN_TEST(test_ModeStateAdvance_CaseAndTriggersAdvance);
    RUN_TEST(test_ModeStateAdvance_IgnoresEquationPatterns);
    RUN_TEST(test_ModeStateAdvance_IgnoresNonMonotonicTime);
    RUN_TEST(test_ModeStateGetSimpleOutput_FalseWhenNoChanges);
    return UNITY_END();
}
