#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"

#include "microlight/json/command_parser.h"
#include "microlight/json/json_buf.h"
#include "microlight/mode_manager.h"
#include "microlight/settings_manager.h"
#include "microlight/usb_manager.h"
// --- Mocks & Stubs ---

// Mock Globals
USBManager usbManager;
ModeManager modeManager;
SettingsManager settingsManager;

// Flash/Storage Mocks
#define TEST_JSON_BUFFER_SIZE 2048
static char mock_flash_buffer[TEST_JSON_BUFFER_SIZE];
static bool mock_flash_write_called = false;
static bool mock_settings_update_called = false;
static bool mock_mode_set_called = false;
static bool mock_enter_dfu_called = false;

// Read/Write buffers for USB mocks
static char mock_usb_read_buffer[TEST_JSON_BUFFER_SIZE];
static bool mock_usb_read_has_data = false;
static char mock_usb_write_buffer[TEST_JSON_BUFFER_SIZE];
static int mock_usb_write_idx = 0;

// Callbacks
int mock_usbCdcReadTask(char usbBuffer[], int bufferLength) {
    if (mock_usb_read_has_data) {
        int len = strlen(mock_usb_read_buffer);
        if (len >= bufferLength) len = bufferLength - 1;
        memcpy(usbBuffer, mock_usb_read_buffer, len);
        usbBuffer[len] = '\0';
        mock_usb_read_has_data = false;
        return len;
    }
    return 0;
}

void mock_usbWriteToSerial(const char usbBuffer[], int bufferLength) {
    if (mock_usb_write_idx + bufferLength < TEST_JSON_BUFFER_SIZE) {
        memcpy(&mock_usb_write_buffer[mock_usb_write_idx], usbBuffer, bufferLength);
        mock_usb_write_idx += bufferLength;
        mock_usb_write_buffer[mock_usb_write_idx] =
            '\0';  // Null terminate for easy string compares
    }
}

// Storage / Logic Mocks
void saveMode(uint8_t mode, const char str[], uint32_t length) {
    mock_flash_write_called = true;
    strncpy(mock_flash_buffer, str, length);
    mock_flash_buffer[length] = '\0';
}
void saveSettings(const char str[], uint32_t length) {
    mock_flash_write_called = true;
    strncpy(mock_flash_buffer, str, length);
    mock_flash_buffer[length] = '\0';
}
void readBulbModeFromMock(uint8_t mode, char buffer[], uint32_t length) {
    strcpy(buffer, "{\"mode\":\"test\"}");
}

// Mocking ModeManager functions
void setMode(ModeManager *manager, Mode *mode, uint8_t index) {
    mock_mode_set_called = true;
}

// Mocking SettingsManager functions
void updateSettings(SettingsManager *manager, ChipSettings *settings) {
    mock_settings_update_called = true;
}
int getSettingsResponse(SettingsManager *manager, char *buffer, uint32_t len) {
    sprintf(buffer, "{\"settings\":\"mock\"}");
    return strlen(buffer);
}

void mock_enter_dfu() {
    mock_enter_dfu_called = true;
}

// --- Setup / Teardown ---

void setUp(void) {
    memset(&usbManager, 0, sizeof(USBManager));
    memset(&modeManager, 0, sizeof(ModeManager));
    memset(&settingsManager, 0, sizeof(SettingsManager));

    // Reset mocks
    mock_enter_dfu_called = false;
    mock_flash_write_called = false;
    mock_settings_update_called = false;
    mock_mode_set_called = false;

    // Reset Buffers
    mock_usb_read_has_data = false;
    mock_usb_write_idx = 0;
    memset(mock_usb_read_buffer, 0, sizeof(mock_usb_read_buffer));
    memset(mock_usb_write_buffer, 0, sizeof(mock_usb_write_buffer));
    memset(mock_flash_buffer, 0, sizeof(mock_flash_buffer));
    initSharedJsonIOBuffer(mock_flash_buffer, TEST_JSON_BUFFER_SIZE);

    // Setup Managers
    modeManager.readSavedMode = readBulbModeFromMock;
}

void tearDown(void) {
}

// --- Tests ---

void test_usbInit_success(void) {
    bool result = usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(mock_usbCdcReadTask, usbManager.usbCdcReadTask);
    TEST_ASSERT_EQUAL_PTR(mock_usbWriteToSerial, usbManager.usbWriteToSerial);
}

void test_usbInit_failure_null_args(void) {
    TEST_ASSERT_FALSE(usbInit(
        NULL,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial));
    TEST_ASSERT_FALSE(usbInit(
        &usbManager,
        NULL,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial));
    TEST_ASSERT_FALSE(usbInit(
        &usbManager,
        &modeManager,
        NULL,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial));
    TEST_ASSERT_FALSE(usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        NULL,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial));
    TEST_ASSERT_FALSE(usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        NULL,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial));
    TEST_ASSERT_FALSE(usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        NULL,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial));
    TEST_ASSERT_FALSE(usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        NULL,
        mock_usbWriteToSerial));
    TEST_ASSERT_FALSE(usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        NULL));
}

// Helper to pump USB task
void pumpUsbTask(void) {
    int limit = 10;
    while (limit-- > 0) {
        usbTask(&usbManager);
    }
}

void test_parse_write_mode_normal(void) {
    // Setup USB Manager
    usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);

    // Simulate incoming "writeMode" JSON
    const char *input =
        "{\"command\":\"writeMode\",\"index\":1,\"mode\":{\"name\":\"test\",\"front\":{\"pattern\":"
        "{\"type\":\"simple\",\"name\":\"test\",\"duration\":1000,\"changeAt\":[{\"ms\":0,"
        "\"output\":\"low\"}]}}}}\n";
    strcpy(mock_usb_read_buffer, input);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    TEST_ASSERT_TRUE(mock_flash_write_called);
    TEST_ASSERT_TRUE(mock_mode_set_called);
    TEST_ASSERT_FALSE(mock_usb_read_has_data);
}

void test_parse_write_mode_transient(void) {
    usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);

    const char *input =
        "{\"command\":\"writeMode\",\"index\":1,\"mode\":{\"name\":\"transientTest\",\"front\":{"
        "\"pattern\":"
        "{\"type\":\"simple\",\"name\":\"test\",\"duration\":1000,\"changeAt\":[{\"ms\":0,"
        "\"output\":\"low\"}]}}}}\n";
    strcpy(mock_usb_read_buffer, input);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    TEST_ASSERT_FALSE(mock_flash_write_called);  // Should NOT save
    TEST_ASSERT_TRUE(mock_mode_set_called);
}

void test_parse_read_mode(void) {
    usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);

    const char *input = "{\"command\":\"readMode\",\"index\":1}\n";
    strcpy(mock_usb_read_buffer, input);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    // Expect response "{"mode":"test"}\n"
    TEST_ASSERT_EQUAL_STRING_LEN("{\"mode\":\"test\"}\n", mock_usb_write_buffer, 16);
}

void test_parse_write_settings(void) {
    usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);

    const char *input = "{\"command\":\"writeSettings\"}\n";
    strcpy(mock_usb_read_buffer, input);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    TEST_ASSERT_TRUE(mock_flash_write_called);
    TEST_ASSERT_TRUE(mock_settings_update_called);
}

void test_parse_read_settings(void) {
    usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);

    const char *input = "{\"command\":\"readSettings\"}\n";
    strcpy(mock_usb_read_buffer, input);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    TEST_ASSERT_EQUAL_STRING_LEN("{\"settings\":\"mock\"}", mock_usb_write_buffer, 18);
}

void test_parse_dfu(void) {
    usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);

    const char *input = "{\"command\":\"dfu\"}\n";
    strcpy(mock_usb_read_buffer, input);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    TEST_ASSERT_TRUE(mock_enter_dfu_called);
}

void test_parse_multiple_commands(void) {
    usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);

    // Command 1
    const char *input1 = "{\"command\":\"writeSettings\"}\n";
    strcpy(mock_usb_read_buffer, input1);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    // Verify 1
    TEST_ASSERT_TRUE(mock_flash_write_called);
    TEST_ASSERT_TRUE(mock_settings_update_called);

    // Reset flags
    mock_flash_write_called = false;
    mock_settings_update_called = false;

    // Command 2
    const char *input2 = "{\"command\":\"readSettings\"}\n";
    strcpy(mock_usb_read_buffer, input2);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    // Verify 2
    TEST_ASSERT_EQUAL_STRING_LEN("{\"settings\":\"mock\"}", mock_usb_write_buffer, 18);
}

void test_malformed_json(void) {
    usbInit(
        &usbManager,
        &modeManager,
        &settingsManager,
        mock_enter_dfu,
        saveSettings,
        saveMode,
        mock_usbCdcReadTask,
        mock_usbWriteToSerial);

    const char *input = "{junk}\n";
    strcpy(mock_usb_read_buffer, input);
    mock_usb_read_has_data = true;

    pumpUsbTask();

    // Expect error response
    TEST_ASSERT_NOT_NULL(strstr(mock_usb_write_buffer, "error"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_malformed_json);
    RUN_TEST(test_parse_dfu);
    RUN_TEST(test_parse_multiple_commands);
    RUN_TEST(test_parse_read_mode);
    RUN_TEST(test_parse_read_settings);
    RUN_TEST(test_parse_write_mode_normal);
    RUN_TEST(test_parse_write_mode_transient);
    RUN_TEST(test_parse_write_settings);
    RUN_TEST(test_usbInit_failure_null_args);
    RUN_TEST(test_usbInit_success);
    return UNITY_END();
}
