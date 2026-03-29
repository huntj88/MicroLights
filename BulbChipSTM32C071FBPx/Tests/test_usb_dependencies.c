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
static bool mock_tud_mounted = true;

// Mock implementations
bool tud_vendor_n_mounted(uint8_t itf) {
    return mock_tud_mounted;
}

uint32_t tud_vendor_n_write_available(uint8_t itf) {
    return mock_tud_write_available;
}

uint32_t tud_vendor_n_write(uint8_t itf, void const *buffer, uint32_t bufsize) {
    if (mock_tud_write_idx + bufsize > sizeof(mock_tud_write_buffer)) {
        bufsize = sizeof(mock_tud_write_buffer) - mock_tud_write_idx;
    }
    memcpy(&mock_tud_write_buffer[mock_tud_write_idx], buffer, bufsize);
    mock_tud_write_idx += bufsize;
    return bufsize;
}

void tud_task_ext(uint32_t timeout_ms, bool in_isr) {
    mock_tud_task_calls++;
}

void tud_task(void) {
    tud_task_ext(0, false);
}

uint32_t tud_vendor_n_write_flush(uint8_t itf) {
    mock_tud_flush_calls++;
    return 0;
}

uint32_t tud_vendor_n_available(uint8_t itf) {
    if (mock_tud_read_idx >= mock_tud_read_len) return 0;
    return mock_tud_read_len - mock_tud_read_idx;
}

uint32_t tud_vendor_n_read(uint8_t itf, void *buffer, uint32_t bufsize) {
    uint32_t available = tud_vendor_n_available(itf);
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
    mock_tud_mounted = true;
    memset(mock_tud_write_buffer, 0, sizeof(mock_tud_write_buffer));
    memset(mock_tud_read_buffer, 0, sizeof(mock_tud_read_buffer));
    usbReadTaskReset();
}

void tearDown(void) {
}

// --- Tests ---

void test_usbWrite_simple(void) {
    const char *data = "Hello World";
    usbWrite(data, strlen(data));

    TEST_ASSERT_EQUAL_STRING_LEN("Hello World", mock_tud_write_buffer, 11);
    TEST_ASSERT_GREATER_THAN(0, mock_tud_task_calls);
    TEST_ASSERT_EQUAL(1, mock_tud_flush_calls);
}

void test_usbWrite_chunked(void) {
    // Simulate limited buffer requiring multiple writes
    const char *data = "1234567890";  // 10 chars
    mock_tud_write_available = 5;     // Report only 5 bytes available
    usbWrite(data, 10);

    TEST_ASSERT_EQUAL_STRING_LEN("1234567890", mock_tud_write_buffer, 10);
    // Should have called task multiple times
    TEST_ASSERT_GREATER_THAN(1, mock_tud_task_calls);
}

void test_usbReadTask_no_data(void) {
    char buf[100];
    int len = usbReadTask(buf, 100);
    TEST_ASSERT_EQUAL(0, len);
}

void test_usbReadTask_full_line(void) {
    const char *input = "command\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    char buf[100];
    int len = usbReadTask(buf, 100);

    TEST_ASSERT_EQUAL(8, len);  // "command\n" is 8 chars
    buf[len] = '\0';            // null terminate for test check
    TEST_ASSERT_EQUAL_STRING("command\n", buf);
}

void test_usbReadTask_split_line(void) {
    // Part 1
    const char *part1 = "part1";
    strcpy(mock_tud_read_buffer, part1);
    mock_tud_read_len = strlen(part1);

    char buf[100];
    int len = usbReadTask(buf, 100);
    TEST_ASSERT_EQUAL(0, len);  // No newline yet

    // Part 2
    mock_tud_read_idx = 0;  // reset read index for new buffer content
    const char *part2 = "part2\n";
    strcpy(mock_tud_read_buffer, part2);
    mock_tud_read_len = strlen(part2);

    len = usbReadTask(buf, 100);
    TEST_ASSERT_EQUAL(11, len);  // length of "part1part2\n"
    buf[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("part1part2\n", buf);
}

void test_usbReadTask_overflow(void) {
    char buf[10];
    // Write 11 chars + newline
    const char *input = "12345678901\n";

    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    int len = usbReadTask(buf, 10);

    TEST_ASSERT_EQUAL(0, len);  // Should reset and return 0

    // Check error was written
    TEST_ASSERT_NOT_NULL(strstr(mock_tud_write_buffer, "payload too long"));
}

void test_usbReadTask_multiple_commands_in_one_read(void) {
    // Simulate two commands arriving in a single USB read
    const char *input = "cmd1\ncmd2\n";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = strlen(input);

    char buf[100];

    // First call should return the first command
    int len = usbReadTask(buf, 100);
    TEST_ASSERT_EQUAL(5, len);  // "cmd1\n"
    buf[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("cmd1\n", buf);

    // Second call should return the second command from leftover buffer
    len = usbReadTask(buf, 100);
    TEST_ASSERT_EQUAL(5, len);  // "cmd2\n"
    buf[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("cmd2\n", buf);

    // Third call should return 0, no more data
    len = usbReadTask(buf, 100);
    TEST_ASSERT_EQUAL(0, len);
}

void test_usbReadTask_leftover_overflow(void) {
    // Scenario: Two commands arrive in one read. The first command is consumed.
    // The second command's leftover bytes cause an overflow when processed
    // with a tiny buffer on the next call.
    //
    // "ab\n" + "cdefghijk" = 12 bytes in one USB read.
    // First call with bufferLength=100 returns "ab\n" (3 bytes).
    // Leftover: "cdefghijk" (9 bytes) sitting in the internal readBuf.
    // Second call with bufferLength=5: processing leftovers, once jsonIndex
    // reaches 5 the guard (jsonIndex + 1 > bufferLength) triggers overflow.
    const char *input = "ab\ncdefghijk";
    strcpy(mock_tud_read_buffer, input);
    mock_tud_read_len = (int)strlen(input);

    char buf[100];

    // First call — consumes "ab\n"
    int len = usbReadTask(buf, 100);
    TEST_ASSERT_EQUAL(3, len);

    // Second call with a small buffer — leftover "cdefghijk" overflows
    mock_tud_write_idx = 0;
    memset(mock_tud_write_buffer, 0, sizeof(mock_tud_write_buffer));
    char smallBuf[5];
    len = usbReadTask(smallBuf, 5);
    TEST_ASSERT_EQUAL(0, len);  // overflow returns 0
    TEST_ASSERT_NOT_NULL(strstr(mock_tud_write_buffer, "payload too long"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_usbReadTask_full_line);
    RUN_TEST(test_usbReadTask_leftover_overflow);
    RUN_TEST(test_usbReadTask_multiple_commands_in_one_read);
    RUN_TEST(test_usbReadTask_no_data);
    RUN_TEST(test_usbReadTask_overflow);
    RUN_TEST(test_usbReadTask_split_line);
    RUN_TEST(test_usbWrite_chunked);
    RUN_TEST(test_usbWrite_simple);
    return UNITY_END();
}
