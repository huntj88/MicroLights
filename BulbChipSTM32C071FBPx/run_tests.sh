#!/bin/bash

# Auto-update test runners
python3 manage_tests.py
if [ $? -ne 0 ]; then
    echo "Test management script failed. Aborting."
    exit 1
fi

# Create output directory
mkdir -p Tests/build
rm -f Tests/build/*

# Common flags
# Include paths:
# - Core/Inc: for project headers (including lwjson_opts.h)
# - libs/Unity/src: for Unity
# - libs/lwjson/lwjson/src/include: for lwjson.h
# - libs/tinyexpr: for tinyexpr.h
CFLAGS="-I Core/Inc -I libs/Unity/src -I libs/lwjson/lwjson/src/include -I libs/tinyexpr -I Tests/mocks -I libs/tinyusb/src -DUNIT_TEST"
UNITY_SRC="libs/Unity/src/unity.c"
LWJSON_SRC="libs/lwjson/lwjson/src/lwjson/lwjson.c"
TINYEXPR_SRC="libs/tinyexpr/tinyexpr.c"

TOTAL_TESTS=0
TOTAL_FAILURES=0
SUITE_ERRORS=0
SHOW_ALL=0

while [ $# -gt 0 ]; do
    case "$1" in
        --verbose)
            SHOW_ALL=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

run_test() {
    local exe_path=$1
    if [ -x "$exe_path" ]; then
        output=$("$exe_path")
        if [ "$SHOW_ALL" -eq 1 ]; then
            echo "$output"
        fi
        
        # Extract counts
        local t=$(echo "$output" | grep -oE '[0-9]+ Tests' | head -n1 | awk '{print $1}')
        local f=$(echo "$output" | grep -oE '[0-9]+ Failures' | head -n1 | awk '{print $1}')
        
        if [ -z "$t" ]; then
            echo "ERROR: Failed to parse test results for $exe_path"
            SUITE_ERRORS=$((SUITE_ERRORS + 1))
        fi

        TOTAL_TESTS=$((TOTAL_TESTS + ${t:-0}))
        TOTAL_FAILURES=$((TOTAL_FAILURES + ${f:-0}))

        if [ "${f:-0}" -gt 0 ]; then
            if [ "$SHOW_ALL" -eq 0 ]; then
                echo "Suite $exe_path reported ${f} failure(s)."
            fi
            echo "Failed asserts (file:line):"
            echo "$output" | grep -E ':[0-9]+:[^:]+:FAIL' | while IFS= read -r fail_line; do
                fail_file=$(echo "$fail_line" | cut -d: -f1)
                fail_line_num=$(echo "$fail_line" | cut -d: -f2)
                fail_test=$(echo "$fail_line" | cut -d: -f3)
                fail_message=$(echo "$fail_line" | cut -d: -f5-)
                [ -z "$fail_message" ] && fail_message="(no failure message)"
                printf '  %s:%s (%s) %s\n' "$fail_file" "$fail_line_num" "$fail_test" "$fail_message"
            done
        fi
    else
        echo "Executable $exe_path not found or not executable (Compilation failed?)."
        SUITE_ERRORS=$((SUITE_ERRORS + 1))
    fi
}

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_chip_state..."; fi
gcc $CFLAGS Tests/microlight/test_chip_state.c $UNITY_SRC $TINYEXPR_SRC -lm -o Tests/build/test_chip_state
run_test ./Tests/build/test_chip_state

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_settings_manager..."; fi
gcc $CFLAGS Tests/microlight/test_settings_manager.c $UNITY_SRC $TINYEXPR_SRC $LWJSON_SRC Core/Src/microlight/json/json_buf.c -lm -o Tests/build/test_settings_manager
run_test ./Tests/build/test_settings_manager

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_mode_manager..."; fi
gcc $CFLAGS Tests/microlight/test_mode_manager.c $UNITY_SRC $LWJSON_SRC Core/Src/microlight/json/command_parser.c Core/Src/microlight/json/mode_parser.c Core/Src/microlight/model/cli_model.c Core/Src/microlight/json/json_buf.c $TINYEXPR_SRC -lm -o Tests/build/test_mode_manager
run_test ./Tests/build/test_mode_manager

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_mode_state..."; fi
gcc $CFLAGS Tests/microlight/model/test_mode_state.c $UNITY_SRC Core/Src/microlight/model/mode_state.c $TINYEXPR_SRC -lm -o Tests/build/test_mode_state
run_test ./Tests/build/test_mode_state

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_button..."; fi
gcc $CFLAGS Tests/microlight/device/test_button.c $UNITY_SRC -o Tests/build/test_button
run_test ./Tests/build/test_button

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_command_parser..."; fi
# Needs lwjson.c linked
gcc $CFLAGS Tests/microlight/json/test_command_parser.c $UNITY_SRC $LWJSON_SRC Core/Src/microlight/model/cli_model.c Core/Src/microlight/json/json_buf.c -o Tests/build/test_command_parser
run_test ./Tests/build/test_command_parser

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_bq25180..."; fi
gcc $CFLAGS Tests/microlight/device/test_bq25180.c $UNITY_SRC $LWJSON_SRC -o Tests/build/test_bq25180
run_test ./Tests/build/test_bq25180

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_storage..."; fi
gcc $CFLAGS Tests/test_storage.c $UNITY_SRC -o Tests/build/test_storage
run_test ./Tests/build/test_storage

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_mcu_dependencies..."; fi
gcc $CFLAGS Tests/test_mcu_dependencies.c $UNITY_SRC -o Tests/build/test_mcu_dependencies
run_test ./Tests/build/test_mcu_dependencies

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_usb_manager..."; fi
gcc $CFLAGS Tests/microlight/test_usb_manager.c $UNITY_SRC $LWJSON_SRC Core/Src/microlight/json/command_parser.c Core/Src/microlight/json/mode_parser.c Core/Src/microlight/json/parser.c Core/Src/microlight/model/cli_model.c Core/Src/microlight/json/json_buf.c Core/Src/microlight/usb_manager.c -lm -o Tests/build/test_usb_manager
run_test ./Tests/build/test_usb_manager

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_i2c_log_decorate..."; fi
gcc $CFLAGS Tests/microlight/test_i2c_log_decorate.c Core/Src/microlight/i2c_log_decorate.c $UNITY_SRC -o Tests/build/test_i2c_log_decorate
run_test ./Tests/build/test_i2c_log_decorate

if [ "$SHOW_ALL" -eq 1 ]; then echo "Compiling and running test_usb_dependencies..."; fi
gcc $CFLAGS Tests/test_usb_dependencies.c Core/Src/usb_dependencies.c $UNITY_SRC -o Tests/build/test_usb_dependencies
run_test ./Tests/build/test_usb_dependencies

echo "---------------------------------------------------"
echo "Final Aggregate Report"
echo "Total Tests: $TOTAL_TESTS"
echo "Total Failures: $TOTAL_FAILURES"

if [ "$SUITE_ERRORS" -ne 0 ]; then
    echo "ERROR: $SUITE_ERRORS test suite(s) failed to start or compile."
    echo "OVERALL STATUS: FAIL"
    exit 1
elif [ "$TOTAL_FAILURES" -eq 0 ]; then
    echo "OVERALL STATUS: PASS"
    exit 0
else
    echo "OVERALL STATUS: FAIL"
    exit 1
fi
