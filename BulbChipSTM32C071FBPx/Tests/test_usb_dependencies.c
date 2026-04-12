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
static int mock_tud_write_calls = 0;
static char mock_tud_read_buffer[1024];
static int mock_tud_read_len = 0;
static int mock_tud_read_idx = 0;
static int mock_tud_task_calls = 0;
static int mock_tud_flush_calls = 0;
static bool mock_tud_mounted = true;
static uint32_t mock_board_time_ms = 0;
static uint32_t mock_board_time_increment_per_task = 0;
static int mock_write_stall_after_first_write_remaining_tasks = -1;
static uint32_t mock_write_recovery_available = 0;

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
    mock_tud_write_calls++;

    if (mock_tud_write_calls == 1 && mock_write_stall_after_first_write_remaining_tasks >= 0) {
        mock_tud_write_available = 0;
    }

    return bufsize;
}

void tud_task_ext(uint32_t timeout_ms, bool in_isr) {
    (void)timeout_ms;
    (void)in_isr;
    mock_tud_task_calls++;
    mock_board_time_ms += mock_board_time_increment_per_task;

    if (mock_write_stall_after_first_write_remaining_tasks > 0) {
        mock_write_stall_after_first_write_remaining_tasks--;
        if (mock_write_stall_after_first_write_remaining_tasks == 0) {
            mock_tud_write_available = mock_write_recovery_available;
        }
    }
}

void tud_task(void) {
    tud_task_ext(0, false);
}

uint32_t tud_vendor_n_write_flush(uint8_t itf) {
    (void)itf;
    mock_tud_flush_calls++;
    return 0;
}

uint32_t board_millis(void) {
    return mock_board_time_ms;
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
    mock_tud_write_calls = 0;
    mock_tud_read_len = 0;
    mock_tud_read_idx = 0;
    mock_tud_task_calls = 0;
    mock_tud_flush_calls = 0;
    mock_tud_mounted = true;
    mock_board_time_ms = 0;
    mock_board_time_increment_per_task = 0;
    mock_write_stall_after_first_write_remaining_tasks = -1;
    mock_write_recovery_available = 0;
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

void test_usbWrite_waits_through_long_backpressure_until_space_returns(void) {
    const char *data = "1234567890";

    mock_tud_write_available = 5;
    mock_write_stall_after_first_write_remaining_tasks = 1500;
    mock_write_recovery_available = 5;
    mock_board_time_increment_per_task = 0;

    usbWrite(data, 10);

    TEST_ASSERT_EQUAL_STRING_LEN("1234567890", mock_tud_write_buffer, 10);
    TEST_ASSERT_GREATER_THAN(1000, mock_tud_task_calls);
}

void test_usbWrite_large_payload_waits_for_second_fifo_window(void) {
    char data[873];
    memset(data, 'A', sizeof(data) - 1);
    data[sizeof(data) - 1] = '\0';

    mock_tud_write_available = 512;
    mock_write_stall_after_first_write_remaining_tasks = 400;
    mock_write_recovery_available = 512;
    mock_board_time_increment_per_task = 1;

    usbWrite(data, strlen(data));

    TEST_ASSERT_EQUAL((int)strlen(data), mock_tud_write_idx);
    TEST_ASSERT_EQUAL_STRING_LEN(data, mock_tud_write_buffer, (int)strlen(data));
    TEST_ASSERT_GREATER_THAN(400, mock_tud_task_calls);
}

void test_usbWrite_stops_after_real_timeout_when_host_never_drains(void) {
    const char *data = "1234567890";

    mock_tud_write_available = 0;
    mock_board_time_increment_per_task = 1;

    usbWrite(data, 10);

    TEST_ASSERT_EQUAL(0, mock_tud_write_idx);
    TEST_ASSERT_GREATER_THAN(0, mock_tud_task_calls);
    TEST_ASSERT_LESS_THAN(1100, mock_tud_task_calls);
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
    RUN_TEST(test_usbWrite_large_payload_waits_for_second_fifo_window);
    RUN_TEST(test_usbWrite_simple);
    RUN_TEST(test_usbWrite_stops_after_real_timeout_when_host_never_drains);
    RUN_TEST(test_usbWrite_waits_through_long_backpressure_until_space_returns);
    return UNITY_END();
}
