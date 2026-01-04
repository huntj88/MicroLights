#include <stdbool.h>
#include <string.h>
#include "device/bq25180.h"
#include "unity.h"

// Mocks
static uint8_t mockRegisters[20];
static uint8_t lastWrittenReg;
static uint8_t lastWrittenValue;
static bool writeCalled = false;

uint8_t mock_readRegister(uint8_t devAddress, uint8_t reg) {
    return mockRegisters[reg];
}

void mock_writeRegister(uint8_t devAddress, uint8_t reg, uint8_t value) {
    lastWrittenReg = reg;
    lastWrittenValue = value;
    writeCalled = true;
    mockRegisters[reg] = value;
}

void mock_writeToSerial(const char *buf, uint32_t count) {
}

// RGB Mocks
bool rgbNotChargingCalled = false;
bool rgbConstantCurrentCalled = false;
bool rgbConstantVoltageCalled = false;
bool rgbDoneChargingCalled = false;

void rgbShowNotCharging(RGBLed *led) {
    rgbNotChargingCalled = true;
}
void rgbShowConstantCurrentCharging(RGBLed *led) {
    rgbConstantCurrentCalled = true;
}
void rgbShowConstantVoltageCharging(RGBLed *led) {
    rgbConstantVoltageCalled = true;
}
void rgbShowDoneCharging(RGBLed *led) {
    rgbDoneChargingCalled = true;
}
void rgbShowUserColor(RGBLed *led, uint8_t r, uint8_t g, uint8_t b) {
}

// Include source under test
#include "../Core/Src/device/bq25180.c"

static BQ25180 charger;
static RGBLed mockLed;

static bool enableTimersCalled = false;
static bool lastEnableTimersArg = false;

void mock_enableTimers(bool enable) {
    enableTimersCalled = true;
    lastEnableTimersArg = enable;
}

void setUp(void) {
    memset(&charger, 0, sizeof(BQ25180));
    memset(&mockLed, 0, sizeof(RGBLed));
    enableTimersCalled = false;
    lastEnableTimersArg = false;

    memset(mockRegisters, 0, sizeof(mockRegisters));
    writeCalled = false;
    lastWrittenReg = 0;
    lastWrittenValue = 0;

    rgbNotChargingCalled = false;
    rgbConstantCurrentCalled = false;
    rgbConstantVoltageCalled = false;
    rgbDoneChargingCalled = false;

    bq25180Init(
        &charger,
        mock_readRegister,
        mock_writeRegister,
        0x6A,
        mock_writeToSerial,
        &mockLed,
        mock_enableTimers);

    // Reset write flag after init, as init performs writes
    writeCalled = false;
    lastWrittenReg = 0;
    lastWrittenValue = 0;
}

void tearDown(void) {
}

void test_ChargerTask_Locks_WhenUnplugged_And_UnplugLockEnabled(void) {
    // 1. Setup initial state: Connected and Charging
    charger.chargingState = constantCurrent;

    // 2. Setup mock registers to return Not Connected
    // STAT0 register: bit 0 is 0 (not connected)
    mockRegisters[BQ25180_STAT0] = 0b00000000;

    // 3. Trigger interrupt handling
    handleChargerInterrupt();

    // 4. Run task with unplugLockEnabled = true
    chargerTask(&charger, 1000, true, false);

    // 5. Verify lock was called (Ship Mode enabled)
    // enableShipMode writes 0b01000001 to BQ25180_SHIP_RST (0x9)
    TEST_ASSERT_TRUE(writeCalled);
    TEST_ASSERT_EQUAL_UINT8(BQ25180_SHIP_RST, lastWrittenReg);
    TEST_ASSERT_EQUAL_UINT8(0b01000001, lastWrittenValue);
}

void test_ChargerTask_DoesNotLock_WhenUnplugged_And_UnplugLockDisabled(void) {
    charger.chargingState = constantCurrent;
    mockRegisters[BQ25180_STAT0] = 0b00000000;  // Not connected

    handleChargerInterrupt();

    chargerTask(&charger, 1000, false, false);  // unplugLockEnabled = false

    TEST_ASSERT_FALSE(writeCalled);
}

void test_ChargerTask_PeriodicallyShowsChargingState(void) {
    charger.chargingState = constantCurrent;
    charger.checkedAtMs = 100;  // Prevent watchdog update

    // Test at (ms & 0x3FF) < 50 (e.g., 1024)
    chargerTask(&charger, 1024, false, true);  // ledEnabled = true
    TEST_ASSERT_TRUE(rgbConstantCurrentCalled);

    // Reset
    rgbConstantCurrentCalled = false;

    // Test at (ms & 0x3FF) >= 50 (e.g., 1100)
    chargerTask(&charger, 1100, false, true);
    TEST_ASSERT_FALSE(rgbConstantCurrentCalled);
}

void test_ChargerTask_UpdatesLed_WhenStateChangesFromNotConnectedToConnected(void) {
    // Initial state: Not Connected
    charger.chargingState = notConnected;
    charger.checkedAtMs = 100;  // Prevent watchdog update

    // New state in registers: Done charging
    mockRegisters[BQ25180_STAT0] = 0b01100000;  // Done charging (Bit 6=1, Bit 5=1)

    handleChargerInterrupt();

    // Run task at time that does NOT trigger periodic flash
    chargerTask(&charger, 1060, false, true);

    // Should update internal state
    TEST_ASSERT_EQUAL(done, charger.chargingState);

    // Should show new state
    TEST_ASSERT_TRUE(rgbDoneChargingCalled);
    TEST_ASSERT_TRUE(enableTimersCalled);
    TEST_ASSERT_TRUE(lastEnableTimersArg);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ChargerTask_DoesNotLock_WhenUnplugged_And_UnplugLockDisabled);
    RUN_TEST(test_ChargerTask_Locks_WhenUnplugged_And_UnplugLockEnabled);
    RUN_TEST(test_ChargerTask_PeriodicallyShowsChargingState);
    RUN_TEST(test_ChargerTask_UpdatesLed_WhenStateChangesFromNotConnectedToConnected);
    return UNITY_END();
}
