#!/bin/bash

# Create output directory
mkdir -p Tests/build

# Common flags
# Include paths:
# - Core/Inc: for project headers (including lwjson_opts.h)
# - libs/Unity/src: for Unity
# - libs/lwjson/lwjson/src/include: for lwjson.h
CFLAGS="-I Core/Inc -I libs/Unity/src -I libs/lwjson/lwjson/src/include -DUNIT_TEST"
UNITY_SRC="libs/Unity/src/unity.c"
LWJSON_SRC="libs/lwjson/lwjson/src/lwjson/lwjson.c"

TOTAL_TESTS=0
TOTAL_FAILURES=0

run_test() {
    local exe_path=$1
    if [ -x "$exe_path" ]; then
        output=$("$exe_path")
        echo "$output"
        
        # Extract counts
        local t=$(echo "$output" | grep -oE '[0-9]+ Tests' | head -n1 | awk '{print $1}')
        local f=$(echo "$output" | grep -oE '[0-9]+ Failures' | head -n1 | awk '{print $1}')
        
        TOTAL_TESTS=$((TOTAL_TESTS + ${t:-0}))
        TOTAL_FAILURES=$((TOTAL_FAILURES + ${f:-0}))
    else
        echo "Executable $exe_path not found or not executable."
    fi
}

echo "Compiling and running test_chip_state..."
gcc $CFLAGS Tests/test_chip_state.c $UNITY_SRC -o Tests/build/test_chip_state
run_test ./Tests/build/test_chip_state

echo "Compiling and running test_mode_manager..."
gcc $CFLAGS Tests/test_mode_manager.c $UNITY_SRC -o Tests/build/test_mode_manager
run_test ./Tests/build/test_mode_manager

echo "Compiling and running test_button..."
gcc $CFLAGS Tests/test_button.c $UNITY_SRC -o Tests/build/test_button
run_test ./Tests/build/test_button

echo "Compiling and running test_command_parser..."
# Needs lwjson.c linked
gcc $CFLAGS Tests/test_command_parser.c $UNITY_SRC $LWJSON_SRC -o Tests/build/test_command_parser
run_test ./Tests/build/test_command_parser

echo "Compiling and running test_bq25180..."
gcc $CFLAGS Tests/test_bq25180.c $UNITY_SRC -o Tests/build/test_bq25180
run_test ./Tests/build/test_bq25180

echo "---------------------------------------------------"
echo "Final Aggregate Report"
echo "Total Tests: $TOTAL_TESTS"
echo "Total Failures: $TOTAL_FAILURES"

if [ "$TOTAL_FAILURES" -eq 0 ]; then
    echo "OVERALL STATUS: PASS"
    exit 0
else
    echo "OVERALL STATUS: FAIL"
    exit 1
fi
