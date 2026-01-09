#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"

#include "json/command_parser.h"
#include "json/json_buf.h"
#include "mode_manager.h"
#include "settings_manager.h"
#include "usb_manager.h"

// --- Mocks & Stubs ---

// Mock Globals
// jsonBuf is defined in json_buf.c which is linked
USBManager usbManager;
ModeManager modeManager;
SettingsManager settingsManager;

// TinyUSB Mocks
static int mock_tud_write_available = 64;
static char mock_tud_write_buffer[1024];
static int mock_tud_write_idx = 0;
static char mock_tud_read_buffer[1024];
static int mock_tud_read_len = 0;
static int mock_tud_read_idx = 0;
static bool mock_tusb_init_called = false;
static bool mock_enter_dfu_called = false;

// Flash/Storage Mocks
static char mock_flash_buffer[JSON_BUFFER_SIZE];
static bool mock_flash_write_called = false;
static bool mock_settings_update_called = false;
static bool mock_mode_set_called = false;

// Stub implementation for dependencies
bool tud_init(int rhport) {
    return true;
}  // Not used directly usually, but tusb_init calls it

bool tusb_rhport_init(uint8_t rhport, const void *rh_init) {
    mock_tusb_init_called = true;
    return true;
}

void tud_task_ext(uint32_t timeout_ms, bool in_isr) {
}

uint32_t tud_cdc_n_write_available(uint8_t itf) {
    return mock_tud_write_available;
}

uint32_t tud_cdc_n_write(uint8_t itf, void const *buffer, uint32_t bufsize) {
    memcpy(&mock_tud_write_buffer[mock_tud_write_idx], buffer, bufsize);
    mock_tud_write_idx += bufsize;
    mock_tud_write_available -= bufsize;
    return bufsize;
}

uint32_t tud_cdc_n_write_flush(uint8_t itf) {
    return 0;
}

bool tud_cdc_n_connected(uint8_t itf) {
    return true;
}

uint32_t tud_cdc_n_available(uint8_t itf) {
    return mock_tud_read_len - mock_tud_read_idx;
}

uint32_t tud_cdc_n_read(uint8_t itf, void *buffer, uint32_t bufsize) {
    uint32_t available = tud_cdc_n_available(itf);
    uint32_t to_read = (bufsize < available) ? bufsize : available;
    memcpy(buffer, &mock_tud_read_buffer[mock_tud_read_idx], to_read);
    mock_tud_read_idx += to_read;
    return to_read;
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
    mock_tusb_init_called = false;
    mock_enter_dfu_called = false;
    mock_flash_write_called = false;
    mock_settings_update_called = false;
    mock_mode_set_called = false;

    // TUD Buffers
    mock_tud_write_available = 64;
    mock_tud_write_idx = 0;
    mock_tud_read_len = 0;
    mock_tud_read_idx = 0;
    memset(mock_tud_write_buffer, 0, sizeof(mock_tud_write_buffer));
    memset(mock_tud_read_buffer, 0, sizeof(mock_tud_read_buffer));
    memset(mock_flash_buffer, 0, sizeof(mock_flash_buffer));
    memset(jsonBuf, 0, JSON_BUFFER_SIZE);

    // Setup Managers
    modeManager.readSavedMode = readBulbModeFromMock;
}

void tearDown(void) {
}

// --- Tests ---

void test_usbInit_success(void) {
    bool result = usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(mock_tusb_init_called);
}

void test_usbInit_failure_null_args(void) {
    TEST_ASSERT_FALSE(usbInit(NULL, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode));
    TEST_ASSERT_FALSE(usbInit(&usbManager, NULL, &settingsManager, mock_enter_dfu, saveSettings, saveMode));
    TEST_ASSERT_FALSE(usbInit(&usbManager, &modeManager, NULL, mock_enter_dfu, saveSettings, saveMode));
    TEST_ASSERT_FALSE(usbInit(&usbManager, &modeManager, &settingsManager, NULL, saveSettings, saveMode));
    TEST_ASSERT_FALSE(usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, NULL, saveMode));
    TEST_ASSERT_FALSE(usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, NULL));
}

void test_usbWriteToSerial(void) {
    const char *msg = "Hello";
    usbWriteToSerial(&usbManager, 0, msg, 5);

    TEST_ASSERT_EQUAL_STRING_LEN("Hello", mock_tud_write_buffer, 5);
}

// Helper to pump USB task until no data available
void pumpUsbTask(void) {
    // Process while data is available to be read
    int limit = 100;  // safety break
    while (tud_cdc_n_available(0) > 0 && limit-- > 0) {
        usbCdcTask(&usbManager);
    }
}

void test_parse_write_mode_normal(void) {
    // Setup USB Manager
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    // Simulate incoming "writeMode" JSON
    const char *input =
        "{\"command\":\"writeMode\",\"index\":1,\"mode\":{\"name\":\"test\",\"front\":{\"pattern\":"
        "{\"type\":\"simple\",\"name\":\"test\",\"duration\":1000,\"changeAt\":[{\"ms\":0,"
        "\"output\":\"low\"}]}}}}\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    TEST_ASSERT_TRUE(mock_flash_write_called);
    TEST_ASSERT_TRUE(mock_mode_set_called);
}

void test_parse_write_mode_transient(void) {
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    // Simulate incoming "transientTest" JSON (writeMode with name="transientTest")
    const char *input =
        "{\"command\":\"writeMode\",\"index\":1,\"mode\":{\"name\":\"transientTest\",\"front\":{"
        "\"pattern\":{\"type\":\"simple\",\"name\":\"test\",\"duration\":1000,\"changeAt\":[{"
        "\"ms\":0,\"output\":\"low\"}]}}}}\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    TEST_ASSERT_FALSE(mock_flash_write_called);  // Check NO flash write
    TEST_ASSERT_TRUE(mock_mode_set_called);      // But mode IS set
}

void test_parse_read_mode(void) {
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    const char *input = "{\"command\":\"readMode\",\"index\":1}\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    // Expect response with mode data
    TEST_ASSERT_EQUAL_STRING_LEN("{\"mode\":\"test\"}\n", mock_tud_write_buffer, 16);
}

void test_parse_write_settings(void) {
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    const char *input = "{\"command\":\"writeSettings\"}\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    TEST_ASSERT_TRUE(mock_flash_write_called);
    TEST_ASSERT_TRUE(mock_settings_update_called);
}

void test_parse_read_settings(void) {
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    const char *input = "{\"command\":\"readSettings\"}\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    TEST_ASSERT_EQUAL_STRING_LEN("{\"settings\":\"mock\"}", mock_tud_write_buffer, 18);
}

void test_parse_dfu(void) {
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    const char *input = "{\"command\":\"dfu\"}\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    TEST_ASSERT_TRUE(mock_enter_dfu_called);
}

void test_parse_multiple_commands(void) {
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    // Send two commands in one stream
    const char *input = "{\"command\":\"writeSettings\"}\n{\"command\":\"readSettings\"}\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    // First command effect
    TEST_ASSERT_TRUE(mock_flash_write_called);
    TEST_ASSERT_TRUE(mock_settings_update_called);

    // Second command effect
    TEST_ASSERT_EQUAL_STRING_LEN("{\"settings\":\"mock\"}", mock_tud_write_buffer, 18);
}

void test_buffer_overflow(void) {
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    // read buffer will not be used if we exceed JSON_BUFFER_SIZE, local variable with JSON_BUFFER_SIZE * 5
    // size to avoid compile error.
    char mock_tud_read_buffer_larger_avoid_compile_error[JSON_BUFFER_SIZE * 5];
    const char *input =
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF"
        "ABCDEFABCDEFABCDEFABCDEFABCDEFABCDEFABCDEF";
    strcpy(mock_tud_read_buffer_larger_avoid_compile_error, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    TEST_ASSERT_NOT_NULL(strstr(mock_tud_write_buffer, "payload too long"));
}

void test_malformed_json(void) {
    usbInit(&usbManager, &modeManager, &settingsManager, mock_enter_dfu, saveSettings, saveMode);

    const char *input = "{junk}\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    pumpUsbTask();

    // Expect error response
    TEST_ASSERT_NOT_NULL(strstr(mock_tud_write_buffer, "error"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_buffer_overflow);
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
    RUN_TEST(test_usbWriteToSerial);
    return UNITY_END();
}
