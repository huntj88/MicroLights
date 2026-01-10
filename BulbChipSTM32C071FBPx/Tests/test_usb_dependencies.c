#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "microlight/model/usb.h"
#include "unity.h"
#include "usb_dependencies.h"

// --- Mocks ---

// TinyUSB Mocks
static uint32_t mock_tud_write_available = 64;
static char mock_tud_write_buffer[1024];
static int mock_tud_write_idx = 0;
static char mock_tud_read_buffer[1024];
static int mock_tud_read_len = 0;
static int mock_tud_read_idx = 0;
static int mock_tud_task_calls = 0;
static int mock_tud_flush_calls = 0;

// Mock implementations
uint32_t tud_cdc_n_write_available(uint8_t itf) {
    return mock_tud_write_available;
}

uint32_t tud_cdc_n_write(uint8_t itf, void const *buffer, uint32_t bufsize) {
    if (mock_tud_write_idx + bufsize > sizeof(mock_tud_write_buffer)) {
        bufsize = sizeof(mock_tud_write_buffer) - mock_tud_write_idx;
    }
    memcpy(&mock_tud_write_buffer[mock_tud_write_idx], buffer, bufsize);
    mock_tud_write_idx += bufsize;
    // mock_tud_write_available -= bufsize; // In simplified mock, we might just reduce what we say
    // is available next time if we want to simulate full buffer, but here we can keep it simple or
    // manual control
    return bufsize;
}

void tud_task_ext(uint32_t timeout_ms, bool in_isr) {
    mock_tud_task_calls++;
}

void tud_task(void) {
    tud_task_ext(0, false);
}

uint32_t tud_cdc_n_write_flush(uint8_t itf) {
    mock_tud_flush_calls++;
    return 0;
}

uint32_t tud_cdc_n_available(uint8_t itf) {
    if (mock_tud_read_idx >= mock_tud_read_len) return 0;
    return mock_tud_read_len - mock_tud_read_idx;
}

uint32_t tud_cdc_n_read(uint8_t itf, void *buffer, uint32_t bufsize) {
    uint32_t available = tud_cdc_n_available(itf);
    uint32_t to_read = (bufsize < available) ? bufsize : available;
    memcpy(buffer, &mock_tud_read_buffer[mock_tud_read_idx], to_read);
    mock_tud_read_idx += to_read;
    return to_read;
}

// --- Setup / Teardown ---

void setUp(void) {
    mock_tud_write_available = 64;
    mock_tud_write_idx = 0;
    mock_tud_read_len = 0;
    mock_tud_read_idx = 0;
    mock_tud_task_calls = 0;
    mock_tud_flush_calls = 0;
    memset(mock_tud_write_buffer, 0, sizeof(mock_tud_write_buffer));
    memset(mock_tud_read_buffer, 0, sizeof(mock_tud_read_buffer));
}

void tearDown(void) {
}

// --- Tests ---

void test_usbWriteToSerial_simple(void) {
    const char *data = "Hello World";
    usbWriteToSerial(data, strlen(data));

    TEST_ASSERT_EQUAL_STRING_LEN("Hello World", mock_tud_write_buffer, 11);
    TEST_ASSERT_GREATER_THAN(0, mock_tud_task_calls);
    TEST_ASSERT_EQUAL(1, mock_tud_flush_calls);
}

void test_usbWriteToSerial_chunked(void) {
    // Simulate limited buffer requiring multiple writes
    const char *data = "1234567890";  // 10 chars
    mock_tud_write_available = 5;     // Report only 5 bytes available
    usbWriteToSerial(data, 10);

    TEST_ASSERT_EQUAL_STRING_LEN("1234567890", mock_tud_write_buffer, 10);
    // Should have called task multiple times
    TEST_ASSERT_GREATER_THAN(1, mock_tud_task_calls);
}

void test_usbCdcReadTask_no_data(void) {
    char buf[100];
    int len = usbCdcReadTask(buf, 100);
    TEST_ASSERT_EQUAL(0, len);
}

void test_usbCdcReadTask_full_line(void) {
    const char *input = "command\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    char buf[100];
    int len = usbCdcReadTask(buf, 100);

    TEST_ASSERT_EQUAL(8, len);  // "command\n" is 8 chars
    buf[len] = '\0';            // null terminate for test check
    TEST_ASSERT_EQUAL_STRING("command\n", buf);
}

void test_usbCdcReadTask_split_line(void) {
    // Part 1
    const char *part1 = "part1";
    strcpy(mock_tud_read_buffer, part1);
    mock_tud_read_len = strlen(part1);

    char buf[100];
    int len = usbCdcReadTask(buf, 100);
    TEST_ASSERT_EQUAL(0, len);  // No newline yet

    // Part 2
    mock_tud_read_idx = 0;  // reset read index for new buffer content
    const char *part2 = "part2\n";
    strcpy(mock_tud_read_buffer, part2);
    mock_tud_read_len = strlen(part2);

    len = usbCdcReadTask(buf, 100);
    TEST_ASSERT_EQUAL(11, len);  // length of "part1part2\n"
    buf[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("part1part2\n", buf);
}

void test_usbCdcReadTask_overflow(void) {
    char buf[10];
    // Write 11 chars + newline
    const char *input = "12345678901\n";

    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    int len = usbCdcReadTask(buf, 10);

    TEST_ASSERT_EQUAL(0, len);  // Should reset and return 0

    // Check error was written
    TEST_ASSERT_NOT_NULL(strstr(mock_tud_write_buffer, "payload too long"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_usbCdcReadTask_full_line);
    RUN_TEST(test_usbCdcReadTask_no_data);
    RUN_TEST(test_usbCdcReadTask_overflow);
    RUN_TEST(test_usbCdcReadTask_split_line);
    RUN_TEST(test_usbWriteToSerial_chunked);
    RUN_TEST(test_usbWriteToSerial_simple);
    return UNITY_END();
}
