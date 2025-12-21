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

echo "Compiling and running test_chip_state..."
gcc $CFLAGS Tests/test_chip_state.c $UNITY_SRC -o Tests/build/test_chip_state
./Tests/build/test_chip_state

echo "Compiling and running test_mode_manager..."
gcc $CFLAGS Tests/test_mode_manager.c $UNITY_SRC -o Tests/build/test_mode_manager
./Tests/build/test_mode_manager

echo "Compiling and running test_button..."
gcc $CFLAGS Tests/test_button.c $UNITY_SRC -o Tests/build/test_button
./Tests/build/test_button

echo "Compiling and running test_command_parser..."
# Needs lwjson.c linked
gcc $CFLAGS Tests/test_command_parser.c $UNITY_SRC $LWJSON_SRC -o Tests/build/test_command_parser
./Tests/build/test_command_parser

echo "Compiling and running test_bq25180..."
gcc $CFLAGS Tests/test_bq25180.c $UNITY_SRC -o Tests/build/test_bq25180
./Tests/build/test_bq25180
