#include <stdbool.h>
#include <string.h>
#include "microlight/i2c_log_decorate.h"
#include "unity.h"

// Mocks identifiers
static uint8_t lastDevAddr;
static uint8_t lastReg;
static uint8_t lastVal;
static bool mockWriteReturn = true;
static bool mockReadReturn = true;

static char logBuffer[256];
static bool logCalled = false;

// Mock functions
bool mock_writeRegisterChecked(uint8_t devAddress, uint8_t reg, uint8_t value) {
    lastDevAddr = devAddress;
    lastReg = reg;
    lastVal = value;
    return mockWriteReturn;
}

bool mock_readRegisters(uint8_t devAddress, uint8_t startReg, uint8_t *buf, size_t len) {
    lastDevAddr = devAddress;
    lastReg = startReg;
    return mockReadReturn;
}

void mock_writeToSerial(const char *buf, uint32_t count) {
    logCalled = true;
    if (count < sizeof(logBuffer)) {
        strncpy(logBuffer, buf, count);
        logBuffer[count] = '\0';
    }
}

// Setup/Teardown
void setUp(void) {
    mockWriteReturn = true;
    mockReadReturn = true;
    logCalled = false;
    lastDevAddr = 0;
    lastReg = 0;
    lastVal = 0;
    memset(logBuffer, 0, sizeof(logBuffer));
}

void tearDown(void) {
}

// Tests for i2cDecoratedWrite

void test_i2cDecoratedWrite_Success_NoLog(void) {
    bool enabled = true;
    mockWriteReturn = true;  // Simulate success

    i2cDecoratedWrite(0x10, 0x20, 0x30, mock_writeRegisterChecked, &enabled, mock_writeToSerial);

    TEST_ASSERT_EQUAL_UINT8(0x10, lastDevAddr);
    TEST_ASSERT_EQUAL_UINT8(0x20, lastReg);
    TEST_ASSERT_EQUAL_UINT8(0x30, lastVal);
    TEST_ASSERT_FALSE(logCalled);
}

void test_i2cDecoratedWrite_Fail_LogDisabled(void) {
    bool enabled = false;
    mockWriteReturn = false;  // Simulate failure

    i2cDecoratedWrite(0x10, 0x20, 0x30, mock_writeRegisterChecked, &enabled, mock_writeToSerial);

    TEST_ASSERT_FALSE(logCalled);
}

void test_i2cDecoratedWrite_Fail_LogEnabled(void) {
    bool enabled = true;
    mockWriteReturn = false;  // Simulate failure

    i2cDecoratedWrite(0x10, 0x20, 0x30, mock_writeRegisterChecked, &enabled, mock_writeToSerial);

    TEST_ASSERT_TRUE(logCalled);
    TEST_ASSERT_NOT_NULL(strstr(logBuffer, "I2C FAIL: Write"));
    TEST_ASSERT_NOT_NULL(strstr(logBuffer, "addr=0x10"));
    TEST_ASSERT_NOT_NULL(strstr(logBuffer, "reg=0x20"));
}

void test_i2cDecoratedWrite_NullRawFunc(void) {
    bool enabled = true;
    // Should handle null raw func gracefully (though caller shouldn't pass null)
    i2cDecoratedWrite(0x10, 0x20, 0x30, NULL, &enabled, mock_writeToSerial);
    // Assuming implementation checks check for null, logic says if (rawFunc && !rawFunc(...)).
    // If rawFunc is null, condition is false -> no log.
    TEST_ASSERT_FALSE(logCalled);
}

// Tests for i2cDecoratedReadRegisters

void test_i2cDecoratedRead_Success_NoLog(void) {
    bool enabled = true;
    mockReadReturn = true;

    uint8_t buf[1];
    bool result = i2cDecoratedReadRegisters(
        0x50, 0x60, buf, 1, mock_readRegisters, &enabled, mock_writeToSerial);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(0x50, lastDevAddr);
    TEST_ASSERT_EQUAL_UINT8(0x60, lastReg);
    TEST_ASSERT_FALSE(logCalled);
}

void test_i2cDecoratedRead_Fail_LogDisabled(void) {
    bool enabled = false;
    mockReadReturn = false;

    uint8_t buf[1];
    bool result = i2cDecoratedReadRegisters(
        0x50, 0x60, buf, 1, mock_readRegisters, &enabled, mock_writeToSerial);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(logCalled);
}

void test_i2cDecoratedRead_Fail_LogEnabled(void) {
    bool enabled = true;
    mockReadReturn = false;

    uint8_t buf[1];
    bool result = i2cDecoratedReadRegisters(
        0x50, 0x60, buf, 1, mock_readRegisters, &enabled, mock_writeToSerial);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(logCalled);
    TEST_ASSERT_NOT_NULL(strstr(logBuffer, "I2C FAIL: ReadMul"));
    TEST_ASSERT_NOT_NULL(strstr(logBuffer, "addr=0x50"));
    TEST_ASSERT_NOT_NULL(strstr(logBuffer, "reg=0x60"));
}

void test_i2cDecoratedRead_NullRawFunc(void) {
    bool enabled = true;
    uint8_t buf[1];
    bool result = i2cDecoratedReadRegisters(0x50, 0x60, buf, 1, NULL, &enabled, mock_writeToSerial);

    // If rawFunc is null then skip if statement, logs error, returns false.
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(logCalled);
    TEST_ASSERT_NOT_NULL(strstr(logBuffer, "ReadMul"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_i2cDecoratedRead_Fail_LogDisabled);
    RUN_TEST(test_i2cDecoratedRead_Fail_LogEnabled);
    RUN_TEST(test_i2cDecoratedRead_NullRawFunc);
    RUN_TEST(test_i2cDecoratedRead_Success_NoLog);
    RUN_TEST(test_i2cDecoratedWrite_Fail_LogDisabled);
    RUN_TEST(test_i2cDecoratedWrite_Fail_LogEnabled);
    RUN_TEST(test_i2cDecoratedWrite_NullRawFunc);
    RUN_TEST(test_i2cDecoratedWrite_Success_NoLog);
    return UNITY_END();
}
