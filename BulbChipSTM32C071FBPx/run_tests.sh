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
CFLAGS="-I Core/Inc -I libs/Unity/src -I libs/lwjson/lwjson/src/include -I libs/tinyexpr -DUNIT_TEST"
UNITY_SRC="libs/Unity/src/unity.c"
LWJSON_SRC="libs/lwjson/lwjson/src/lwjson/lwjson.c"
TINYEXPR_SRC="libs/tinyexpr/tinyexpr.c"

TOTAL_TESTS=0
TOTAL_FAILURES=0
SUITE_ERRORS=0

run_test() {
    local exe_path=$1
    if [ -x "$exe_path" ]; then
        output=$("$exe_path")
        echo "$output"
        
        # Extract counts
        local t=$(echo "$output" | grep -oE '[0-9]+ Tests' | head -n1 | awk '{print $1}')
        local f=$(echo "$output" | grep -oE '[0-9]+ Failures' | head -n1 | awk '{print $1}')
        
        if [ -z "$t" ]; then
            echo "ERROR: Failed to parse test results for $exe_path"
            SUITE_ERRORS=$((SUITE_ERRORS + 1))
        fi

        TOTAL_TESTS=$((TOTAL_TESTS + ${t:-0}))
        TOTAL_FAILURES=$((TOTAL_FAILURES + ${f:-0}))
    else
        echo "Executable $exe_path not found or not executable (Compilation failed?)."
        SUITE_ERRORS=$((SUITE_ERRORS + 1))
    fi
}

echo "Compiling and running test_chip_state..."
gcc $CFLAGS Tests/test_chip_state.c $UNITY_SRC $TINYEXPR_SRC -lm -o Tests/build/test_chip_state
run_test ./Tests/build/test_chip_state

echo "Compiling and running test_settings_manager..."
gcc $CFLAGS Tests/test_settings_manager.c $UNITY_SRC $TINYEXPR_SRC -lm -o Tests/build/test_settings_manager
run_test ./Tests/build/test_settings_manager

echo "Compiling and running test_mode_manager..."
gcc $CFLAGS Tests/test_mode_manager.c $UNITY_SRC $LWJSON_SRC Core/Src/json/command_parser.c Core/Src/json/mode_parser.c Core/Src/model/cli_model.c Core/Src/json/json_buf.c $TINYEXPR_SRC -lm -o Tests/build/test_mode_manager
run_test ./Tests/build/test_mode_manager

echo "Compiling and running test_mode_state..."
gcc $CFLAGS Tests/test_mode_state.c $UNITY_SRC Core/Src/model/mode_state.c $TINYEXPR_SRC -lm -o Tests/build/test_mode_state
run_test ./Tests/build/test_mode_state

echo "Compiling and running test_button..."
gcc $CFLAGS Tests/test_button.c $UNITY_SRC -o Tests/build/test_button
run_test ./Tests/build/test_button

echo "Compiling and running test_command_parser..."
# Needs lwjson.c linked
gcc $CFLAGS Tests/test_command_parser.c $UNITY_SRC $LWJSON_SRC Core/Src/model/cli_model.c Core/Src/json/json_buf.c -o Tests/build/test_command_parser
run_test ./Tests/build/test_command_parser

echo "Compiling and running test_bq25180..."
gcc $CFLAGS Tests/test_bq25180.c $UNITY_SRC -o Tests/build/test_bq25180
run_test ./Tests/build/test_bq25180

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
