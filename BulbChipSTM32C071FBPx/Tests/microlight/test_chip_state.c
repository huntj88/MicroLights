#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "unity.h"

// Include headers required by chip_state.c
#include "microlight/chip_state.h"
#include "microlight/device/bq25180.h"
#include "microlight/device/button.h"
#include "microlight/device/mc3479.h"
#include "microlight/device/rgb_led.h"
#include "microlight/mode_manager.h"
#include "microlight/settings_manager.h"

// Mock Data
static ModeManager mockModeManager;
static ChipSettings mockSettings;
static Button mockButton;
static BQ25180 mockCharger;
static MC3479 mockAccel;
static RGBLed mockCaseLed;
static ChipState state;
static ChipDependencies mockDeps;

static uint32_t mockMillisPerTick = 10;
static uint32_t mockMsPerTickMultiplier = 0;
static char lastSerialOutput[100];
static bool chipTickTimerEnabled = false;
static bool caseLedTimerEnabled = false;
static bool frontLedTimerEnabled = false;
static bool usbClockEnabled = false;
static uint32_t chipTickTimerCallCount = 0;
static uint32_t caseLedTimerCallCount = 0;
static uint32_t frontLedTimerCallCount = 0;
static uint32_t usbClockCallCount = 0;
static bool chargerTaskCalled = false;
static ChargerTaskFlags lastChargerFlags;
static bool lastModeTaskCanUpdateCaseLed = false;
static ModeOutputs nextModeOutputs;
static uint32_t enterStandbyModeCallCount = 0;
static uint32_t enterStopModeCallCount = 0;
static uint32_t autoOffTimerEnableCallCount = 0;
static bool autoOffTimerEnabled = false;
static uint16_t lastStopWakeIntervalSeconds = 0;
static uint16_t lastLockThresholdMinutes = 0;
static bool mockWakeFromButton = false;
static bool mockSystemResetCalled = false;
static uint32_t mc3479DisableCallCount = 0;

// Mock Function Implementations
uint32_t mock_convertTicksToMs(uint32_t ticks) {
    if (mockMsPerTickMultiplier == 0) {
        mockMsPerTickMultiplier = (uint32_t)(mockMillisPerTick * 1048576);  // 2^20 for fixed point
    }
    return (uint32_t)(((uint64_t)ticks * mockMsPerTickMultiplier) >> 20);
}

void mock_writeUsbSerial(const char *buf, size_t count) {
    strncpy(lastSerialOutput, buf, count);
    lastSerialOutput[count] = '\0';
}

// External Mocks (Functions called by chip_state.c)
enum ChargeState mockChargeState = notConnected;
enum ChargeState getChargingState(BQ25180 *dev, uint32_t milliseconds) {
    (void)milliseconds;
    return mockChargeState;
}

uint8_t lastLoadedModeIndex = 255;
void loadMode(ModeManager *manager, uint8_t index) {
    lastLoadedModeIndex = index;
    manager->currentModeIndex = index;
}

void mock_enableChipTickTimer(bool enable) {
    chipTickTimerEnabled = enable;
    chipTickTimerCallCount++;
}

void mock_enableCaseLedTimer(bool enable) {
    caseLedTimerEnabled = enable;
    caseLedTimerCallCount++;
}

void mock_enableFrontLedTimer(bool enable) {
    frontLedTimerEnabled = enable;
    frontLedTimerCallCount++;
}

void mock_enableAutoOffTimer(bool enable) {
    autoOffTimerEnabled = enable;
    autoOffTimerEnableCallCount++;
}

void mock_enableUsbClock(bool enable) {
    usbClockEnabled = enable;
    usbClockCallCount++;
}

enum ButtonResult mockButtonResult = ignore;
enum ButtonResult buttonInputTask(Button *button, uint32_t ms, bool interruptTriggered) {
    return mockButtonResult;
}

bool mockRgbShowSuccessCalled = false;
void rgbShowSuccess(RGBLed *led) {
    mockRgbShowSuccessCalled = true;
}

bool mockLockCalled = false;
void lock(BQ25180 *dev) {
    mockLockCalled = true;
}

void rgbTransientTask(RGBLed *led, uint32_t ms) {
}
void mc3479Disable(MC3479 *dev) {
    (void)dev;
    mc3479DisableCallCount++;
}
void mc3479Task(MC3479 *dev, uint32_t ms) {
}
void chargerTask(BQ25180 *dev, uint32_t ms, ChargerTaskFlags flags) {
    (void)dev;
    (void)ms;
    chargerTaskCalled = true;
    lastChargerFlags = flags;
}
ModeOutputs modeTask(
    ModeManager *manager, uint32_t ms, bool canUpdateCaseLed, uint8_t equationEvalIntervalMs) {
    (void)manager;
    (void)ms;
    lastModeTaskCanUpdateCaseLed = canUpdateCaseLed;
    (void)equationEvalIntervalMs;
    return nextModeOutputs;
}

bool isFakeOff(ModeManager *manager) {
    return manager->currentModeIndex == FAKE_OFF_MODE_INDEX;
}

bool mockIsEvaluatingButtonPress = false;
bool isEvaluatingButtonPress(Button *button) {
    return mockIsEvaluatingButtonPress;
}

void fakeOffMode(ModeManager *manager) {
    // isFakeOff() derives state from manager->currentModeIndex (set by loadMode).
    loadMode(manager, FAKE_OFF_MODE_INDEX);
}

void enterStandbyMode(void) {
    enterStandbyModeCallCount++;
}

void enterStopModeWithRtcAlarm(uint16_t wakeIntervalSeconds) {
    enterStopModeCallCount++;
    lastStopWakeIntervalSeconds = wakeIntervalSeconds;
}

bool mock_wasWakeFromButton(void) {
    bool didWakeFromButton = mockWakeFromButton;
    mockWakeFromButton = false;
    return didWakeFromButton;
}

bool mock_waitForButtonWakeOrAutoLock(uint16_t wakeIntervalSeconds, uint16_t lockThresholdMinutes) {
    uint32_t elapsedSeconds = 0;
    uint32_t lockThresholdSeconds = (uint32_t)lockThresholdMinutes * 60U;

    lastLockThresholdMinutes = lockThresholdMinutes;

    while (true) {
        enterStopModeWithRtcAlarm(wakeIntervalSeconds);
        if (mock_wasWakeFromButton()) {
            return true;
        }

        elapsedSeconds += wakeIntervalSeconds;
        if (elapsedSeconds >= lockThresholdSeconds) {
            return false;
        }
    }
}

void mock_systemReset(void) {
    mockSystemResetCalled = true;
}

// Include the source files under test to access static state
#include "../../Core/Src/microlight/chip_state.c"
#include "../../Core/Src/microlight/model/mode_state.c"

// Setup and Teardown
void setUp(void) {
    memset(&mockModeManager, 0, sizeof(ModeManager));
    memset(&mockSettings, 0, sizeof(ChipSettings));
    memset(&mockButton, 0, sizeof(Button));
    memset(&mockCharger, 0, sizeof(BQ25180));
    memset(&mockAccel, 0, sizeof(MC3479));
    memset(&mockCaseLed, 0, sizeof(RGBLed));

    mockMillisPerTick = 10.0f;
    mockMsPerTickMultiplier = 0;
    memset(lastSerialOutput, 0, sizeof(lastSerialOutput));
    chipTickTimerEnabled = false;
    caseLedTimerEnabled = false;
    frontLedTimerEnabled = false;
    usbClockEnabled = false;
    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;
    usbClockCallCount = 0;
    chargerTaskCalled = false;
    memset(&lastChargerFlags, 0, sizeof(lastChargerFlags));
    lastModeTaskCanUpdateCaseLed = false;
    enterStandbyModeCallCount = 0;
    enterStopModeCallCount = 0;
    autoOffTimerEnableCallCount = 0;
    autoOffTimerEnabled = false;
    lastStopWakeIntervalSeconds = 0;
    lastLockThresholdMinutes = 0;
    mockWakeFromButton = false;
    mockSystemResetCalled = false;
    mc3479DisableCallCount = 0;
    nextModeOutputs = (ModeOutputs){
        .frontValid = false,
        .caseValid = false,
        .frontType = BULB,
    };

    mockChargeState = notConnected;
    lastLoadedModeIndex = 255;
    mockButtonResult = ignore;
    mockRgbShowSuccessCalled = false;
    mockLockCalled = false;
    mockIsEvaluatingButtonPress = false;

    state = (ChipState){0};  // Reset internal state

    mockDeps = (ChipDependencies){
        .modeManager = &mockModeManager,
        .settings = &mockSettings,
        .button = &mockButton,
        .chargerIC = &mockCharger,
        .accel = &mockAccel,
        .caseLed = &mockCaseLed,
        .enableChipTickTimer = mock_enableChipTickTimer,
        .enableCaseLedTimer = mock_enableCaseLedTimer,
        .enableFrontLedTimer = mock_enableFrontLedTimer,
        .enableAutoOffTimer = mock_enableAutoOffTimer,
        .enableUsbClock = mock_enableUsbClock,
        .enterStandbyMode = enterStandbyMode,
        .waitForButtonWakeOrAutoLock = mock_waitForButtonWakeOrAutoLock,
        .systemReset = mock_systemReset,
        .log = mock_writeUsbSerial,
    };
}

void tearDown(void) {
}

// Test Cases

void test_ConfigureChipState_WhenNotCharging_LoadsModeZero(void) {
    mockChargeState = notConnected;
    configureChipState(&state, mockDeps);

    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);
}

void test_ConfigureChipState_WhenCharging_EntersFakeOff(void) {
    mockChargeState = constantCurrent;
    configureChipState(&state, mockDeps);

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Clicked_CyclesToNextMode(void) {
    configureChipState(&state, mockDeps);

    mockModeManager.currentModeIndex = 1;
    mockSettings.modeCount = 5;
    mockButtonResult = clicked;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(mockRgbShowSuccessCalled);
    TEST_ASSERT_EQUAL_UINT8(2, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Clicked_WrapsModeIndex(void) {
    configureChipState(&state, mockDeps);

    mockModeManager.currentModeIndex = 4;
    mockSettings.modeCount = 5;
    mockButtonResult = clicked;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Shutdown_EntersStandby_WhenNotCharging(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = manualShutdownOnly;
    mockButtonResult = shutdown;
    mockChargeState = notConnected;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT32(1, enterStandbyModeCallCount);
    TEST_ASSERT_EQUAL_UINT32(0, enterStopModeCallCount);
    TEST_ASSERT_EQUAL_UINT32(1, autoOffTimerEnableCallCount);
    TEST_ASSERT_FALSE(autoOffTimerEnabled);
}

void test_StateTask_ButtonResult_Shutdown_EntersStopMode_WhenAutoLockEnabled(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = autoOffAndAutoLock;
    mockSettings.minutesUntilLockAfterAutoOff = 2;
    mockButtonResult = shutdown;
    mockChargeState = notConnected;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT32(2, enterStopModeCallCount);
    TEST_ASSERT_EQUAL_UINT16(60, lastStopWakeIntervalSeconds);
    TEST_ASSERT_EQUAL_UINT16(2, lastLockThresholdMinutes);
    TEST_ASSERT_TRUE(mockLockCalled);
    TEST_ASSERT_FALSE(mockSystemResetCalled);
    TEST_ASSERT_EQUAL_UINT32(1, autoOffTimerEnableCallCount);
    TEST_ASSERT_FALSE(autoOffTimerEnabled);
}

void test_StateTask_ButtonResult_Shutdown_DisablesActiveTimers_BeforeLowPower(void) {
    configureChipState(&state, mockDeps);

    state.lastChipTickEnabled = true;
    state.lastCasePwmEnabled = true;
    state.lastFrontPwmEnabled = true;
    chipTickTimerEnabled = true;
    caseLedTimerEnabled = true;
    frontLedTimerEnabled = true;

    mockSettings.shutdownPolicy = autoOffAndAutoLock;
    mockSettings.minutesUntilLockAfterAutoOff = 1;
    mockButtonResult = shutdown;
    mockChargeState = notConnected;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_FALSE(chipTickTimerEnabled);
    TEST_ASSERT_FALSE(caseLedTimerEnabled);
    TEST_ASSERT_FALSE(frontLedTimerEnabled);
    TEST_ASSERT_EQUAL_UINT32(1, mc3479DisableCallCount);
}

void test_StateTask_ButtonResult_Shutdown_ImmediateLock_SkipsStopMode(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = autoOffAndAutoLock;
    mockSettings.minutesUntilLockAfterAutoOff = 0;
    mockButtonResult = shutdown;
    mockChargeState = notConnected;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(mockLockCalled);
    TEST_ASSERT_EQUAL_UINT32(0, enterStopModeCallCount);
}

void test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenCharging(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = autoOffAndAutoLock;
    mockSettings.minutesUntilLockAfterAutoOff = 2;
    mockButtonResult = shutdown;
    mockChargeState = constantCurrent;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
    TEST_ASSERT_EQUAL_UINT32(0, enterStandbyModeCallCount);
    TEST_ASSERT_EQUAL_UINT32(0, enterStopModeCallCount);
}

void test_StateTask_Shutdown_ChargeLedEnabled_WhenCharging(void) {
    configureChipState(&state, mockDeps);

    mockChargeState = constantCurrent;
    mockButtonResult = shutdown;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(chargerTaskCalled);
    TEST_ASSERT_TRUE(lastChargerFlags.chargeLedEnabled);
}

void test_StateTask_ChargeLedDisabled_WhenNotCharging(void) {
    configureChipState(&state, mockDeps);

    mockChargeState = notConnected;
    mockModeManager.currentModeIndex = FAKE_OFF_MODE_INDEX;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(chargerTaskCalled);
    TEST_ASSERT_FALSE(lastChargerFlags.chargeLedEnabled);
}

void test_StateTask_ModeTask_DisabledCaseLed_WhenFakeOff(void) {
    configureChipState(&state, mockDeps);

    mockModeManager.currentModeIndex = FAKE_OFF_MODE_INDEX;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_FALSE(lastModeTaskCanUpdateCaseLed);
}

void test_StateTask_ButtonInterrupt_EnablesCasePwm(void) {
    configureChipState(&state, mockDeps);

    mockChargeState = notConnected;
    mockIsEvaluatingButtonPress = false;
    nextModeOutputs = (ModeOutputs){
        .frontValid = false,
        .caseValid = false,
        .frontType = BULB,
    };

    stateTask(&state, 0, (StateTaskFlags){.buttonInterruptTriggered = true});

    TEST_ASSERT_TRUE(caseLedTimerEnabled);
}

void test_StateTask_ButtonInterrupt_EnablesChipTickTimer_WhenFakeOff(void) {
    configureChipState(&state, mockDeps);

    mockChargeState = notConnected;
    mockModeManager.currentModeIndex = FAKE_OFF_MODE_INDEX;
    mockIsEvaluatingButtonPress = false;
    nextModeOutputs = (ModeOutputs){
        .frontValid = false,
        .caseValid = false,
        .frontType = BULB,
    };

    // First call in fake-off with no button activity — chip tick should be disabled
    stateTask(&state, 0, (StateTaskFlags){0});
    TEST_ASSERT_FALSE(chipTickTimerEnabled);

    // Button interrupt fires — chip tick should be enabled to service the press
    chipTickTimerCallCount = 0;
    stateTask(&state, 10, (StateTaskFlags){.buttonInterruptTriggered = true});
    TEST_ASSERT_TRUE(chipTickTimerEnabled);
    TEST_ASSERT_GREATER_THAN_UINT32(0, chipTickTimerCallCount);
}

void test_StateTask_ButtonResult_Lock_LocksCharger(void) {
    configureChipState(&state, mockDeps);

    mockButtonResult = lockOrHardwareReset;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(mockLockCalled);
}

void test_AutoOffTimer_DoesNothing_WhenManualShutdownOnly(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = manualShutdownOnly;
    mockSettings.minutesUntilAutoOff = 1;  // 1 minute
    mockChargeState = notConnected;
    state.ticksSinceLastUserActivity = 7;  // Exceeds 6

    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});

    TEST_ASSERT_EQUAL_UINT32(0, enterStandbyModeCallCount);
    TEST_ASSERT_EQUAL_UINT32(0, enterStopModeCallCount);
    TEST_ASSERT_FALSE(mockLockCalled);
}

void test_AutoOffTimer_EntersStandby_AfterTimeout_WhenAutoOffEnabled(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = autoOffNoAutoLock;
    mockSettings.minutesUntilAutoOff = 1;
    mockChargeState = notConnected;
    state.ticksSinceLastUserActivity = 7;

    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});

    TEST_ASSERT_EQUAL_UINT32(1, enterStandbyModeCallCount);
    TEST_ASSERT_EQUAL_UINT32(0, enterStopModeCallCount);
    TEST_ASSERT_EQUAL_UINT32(1, autoOffTimerEnableCallCount);
    TEST_ASSERT_FALSE(autoOffTimerEnabled);
}

void test_AutoOffTimer_AutoLock_StopsAutoOffTimer_BeforeStopMode(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = autoOffAndAutoLock;
    mockSettings.minutesUntilAutoOff = 1;
    mockSettings.minutesUntilLockAfterAutoOff = 2;
    mockChargeState = notConnected;
    state.ticksSinceLastUserActivity = 6;

    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});

    TEST_ASSERT_EQUAL_UINT32(1, autoOffTimerEnableCallCount);
    TEST_ASSERT_FALSE(autoOffTimerEnabled);
    TEST_ASSERT_EQUAL_UINT32(2, enterStopModeCallCount);
}

void test_Settings_ModeCount_LimitsModeCycling(void) {
    configureChipState(&state, mockDeps);

    // Case 1: Mode Count 3, Current 2 -> Should wrap to 0
    mockSettings.modeCount = 3;
    mockModeManager.currentModeIndex = 2;
    mockButtonResult = clicked;

    stateTask(&state, 0, (StateTaskFlags){0});
    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);

    // Case 2: Mode Count 5, Current 2 -> Should go to 3
    mockSettings.modeCount = 5;
    mockModeManager.currentModeIndex = 2;
    mockButtonResult = clicked;

    stateTask(&state, 0, (StateTaskFlags){0});
    TEST_ASSERT_EQUAL_UINT8(3, lastLoadedModeIndex);
}

void test_Settings_MinutesUntilAutoOff_ChangesTimeout(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = autoOffNoAutoLock;
    mockChargeState = notConnected;

    // Case 1: 1 Minute (6 ticks)
    mockSettings.minutesUntilAutoOff = 1;

    // Just below threshold (starts at 5, increments to 6 inside interrupt. 6 > 6 is False)
    state.ticksSinceLastUserActivity = 5;
    lastLoadedModeIndex = 0;  // Reset

    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});

    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);  // No change

    // Just above threshold (starts at 6, increments to 7 inside interrupt. 7 > 6 is True)
    state.ticksSinceLastUserActivity = 6;
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_EQUAL_UINT32(1, enterStandbyModeCallCount);

    // Case 2: 2 Minutes (12 ticks)
    mockSettings.minutesUntilAutoOff = 2;
    enterStandbyModeCallCount = 0;

    // Above 1 min threshold, but below 2 min (starts at 11, increments to 12. 12 > 12 is False)
    state.ticksSinceLastUserActivity = 11;
    lastLoadedModeIndex = 0;  // Reset
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_EQUAL_UINT32(0, enterStandbyModeCallCount);  // No change

    // Above 2 min threshold (starts at 12, increments to 13. 13 > 12 is True)
    state.ticksSinceLastUserActivity = 12;
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_EQUAL_UINT32(1, enterStandbyModeCallCount);  // Changed
}

void test_Settings_MinutesUntilLockAfterAutoOff_ChangesStopLockTimeout(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = autoOffAndAutoLock;
    mockChargeState = notConnected;

    mockSettings.minutesUntilLockAfterAutoOff = 1;
    mockLockCalled = false;
    mockButtonResult = shutdown;

    stateTask(&state, 0, (StateTaskFlags){0});
    TEST_ASSERT_TRUE(mockLockCalled);

    mockSettings.minutesUntilLockAfterAutoOff = 2;
    mockLockCalled = false;
    enterStopModeCallCount = 0;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(mockLockCalled);
    TEST_ASSERT_EQUAL_UINT32(2, enterStopModeCallCount);
}

void test_StateTask_StopMode_ButtonWake_ResetsSystem(void) {
    configureChipState(&state, mockDeps);

    mockSettings.shutdownPolicy = autoOffAndAutoLock;
    mockSettings.minutesUntilLockAfterAutoOff = 2;
    mockButtonResult = shutdown;
    mockChargeState = notConnected;
    mockWakeFromButton = true;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(mockSystemResetCalled);
    TEST_ASSERT_FALSE(mockLockCalled);
}

void test_TimerPolicy_SkipsRedundantCalls(void) {
    configureChipState(&state, mockDeps);

    mockChargeState = notConnected;
    mockIsEvaluatingButtonPress = false;
    nextModeOutputs = (ModeOutputs){
        .frontValid = false,
        .caseValid = false,
        .frontType = BULB,
    };

    // First call should invoke all three timer callbacks (state transitions from init)
    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;

    stateTask(&state, 0, (StateTaskFlags){0});

    uint32_t firstChipTickCalls = chipTickTimerCallCount;
    uint32_t firstCaseCalls = caseLedTimerCallCount;
    uint32_t firstFrontCalls = frontLedTimerCallCount;
    TEST_ASSERT_GREATER_THAN_UINT32(0, firstChipTickCalls);

    // Second call with same state should NOT invoke callbacks again
    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;

    stateTask(&state, 10, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, chipTickTimerCallCount, "chipTick timer called redundantly");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, caseLedTimerCallCount, "caseLed timer called redundantly");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, frontLedTimerCallCount, "frontLed timer called redundantly");

    // Change state: enable front RGB — only front timer should be called
    nextModeOutputs = (ModeOutputs){
        .frontValid = true,
        .caseValid = false,
        .frontType = RGB,
    };

    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;

    stateTask(&state, 20, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, chipTickTimerCallCount, "chipTick timer should not change");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, caseLedTimerCallCount, "caseLed timer should not change");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        1, frontLedTimerCallCount, "frontLed timer should be called once");
    TEST_ASSERT_TRUE(frontLedTimerEnabled);
}

void test_TimerPolicy_FrontBulbType_DisablesFrontTimer(void) {
    configureChipState(&state, mockDeps);

    mockChargeState = notConnected;
    mockIsEvaluatingButtonPress = false;

    // frontValid=true but frontType=BULB — front PWM should stay disabled
    nextModeOutputs = (ModeOutputs){
        .frontValid = true,
        .caseValid = false,
        .frontType = BULB,
    };

    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;

    stateTask(&state, 0, (StateTaskFlags){0});

    // Front timer callback fires once (initial state transition from init)
    // but the timer should be DISABLED because BULB type bypasses PWM
    TEST_ASSERT_FALSE_MESSAGE(frontLedTimerEnabled, "front timer should be disabled for BULB type");

    // Now switch to RGB — front timer should become enabled
    nextModeOutputs = (ModeOutputs){
        .frontValid = true,
        .caseValid = false,
        .frontType = RGB,
    };

    frontLedTimerCallCount = 0;

    stateTask(&state, 10, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        1, frontLedTimerCallCount, "frontLed timer should be called on BULB→RGB transition");
    TEST_ASSERT_TRUE_MESSAGE(frontLedTimerEnabled, "front timer should be enabled for RGB type");

    // Switch back to BULB — front timer should be disabled again
    nextModeOutputs = (ModeOutputs){
        .frontValid = true,
        .caseValid = false,
        .frontType = BULB,
    };

    frontLedTimerCallCount = 0;

    stateTask(&state, 20, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        1, frontLedTimerCallCount, "frontLed timer should be called on RGB→BULB transition");
    TEST_ASSERT_FALSE_MESSAGE(
        frontLedTimerEnabled, "front timer should be disabled after RGB→BULB transition");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_AutoOffTimer_AutoLock_StopsAutoOffTimer_BeforeStopMode);
    RUN_TEST(test_AutoOffTimer_DoesNothing_WhenManualShutdownOnly);
    RUN_TEST(test_AutoOffTimer_EntersStandby_AfterTimeout_WhenAutoOffEnabled);
    RUN_TEST(test_ConfigureChipState_WhenCharging_EntersFakeOff);
    RUN_TEST(test_ConfigureChipState_WhenNotCharging_LoadsModeZero);
    RUN_TEST(test_Settings_MinutesUntilAutoOff_ChangesTimeout);
    RUN_TEST(test_Settings_MinutesUntilLockAfterAutoOff_ChangesStopLockTimeout);
    RUN_TEST(test_Settings_ModeCount_LimitsModeCycling);
    RUN_TEST(test_StateTask_ButtonInterrupt_EnablesCasePwm);
    RUN_TEST(test_StateTask_ButtonInterrupt_EnablesChipTickTimer_WhenFakeOff);
    RUN_TEST(test_StateTask_ButtonResult_Clicked_CyclesToNextMode);
    RUN_TEST(test_StateTask_ButtonResult_Clicked_WrapsModeIndex);
    RUN_TEST(test_StateTask_ButtonResult_Lock_LocksCharger);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_DisablesActiveTimers_BeforeLowPower);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenCharging);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_EntersStandby_WhenNotCharging);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_EntersStopMode_WhenAutoLockEnabled);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_ImmediateLock_SkipsStopMode);
    RUN_TEST(test_StateTask_ChargeLedDisabled_WhenNotCharging);
    RUN_TEST(test_StateTask_ModeTask_DisabledCaseLed_WhenFakeOff);
    RUN_TEST(test_StateTask_Shutdown_ChargeLedEnabled_WhenCharging);
    RUN_TEST(test_StateTask_StopMode_ButtonWake_ResetsSystem);
    RUN_TEST(test_TimerPolicy_FrontBulbType_DisablesFrontTimer);
    RUN_TEST(test_TimerPolicy_SkipsRedundantCalls);
    return UNITY_END();
}
