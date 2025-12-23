#!/bin/bash

# Check if cppcheck is installed
if ! command -v cppcheck &> /dev/null; then
    echo "cppcheck could not be found. Please install it (e.g., sudo apt install cppcheck)."
    exit 1
fi

# Check if clang-tidy is installed
if ! command -v clang-tidy &> /dev/null; then
    echo "clang-tidy could not be found. Please install it (e.g., sudo apt install clang-tidy)."
    exit 1
fi

echo "Running cppcheck..."

# Define include paths
INCLUDES="-I Core/Inc \
          -I Drivers/STM32C0xx_HAL_Driver/Inc \
          -I Drivers/CMSIS/Device/ST/STM32C0xx/Include \
          -I Drivers/CMSIS/Include \
          -I libs/lwjson/lwjson/src/include \
          -I libs/tinyusb/src"

# Run cppcheck
# --enable=warning,performance,portability: Enable useful checks
# --error-exitcode=1: Fail build on error
# --suppress=missingIncludeSystem: Ignore missing system headers
# --inline-suppr: Allow inline suppressions in code
cppcheck --enable=warning,performance,portability \
         --error-exitcode=1 \
         --suppress=missingIncludeSystem \
         --suppress=toomanyconfigs \
         --inline-suppr \
         --quiet \
         $INCLUDES \
         Core/Src Core/Inc

echo "Running clang-tidy..."

# Find all C source files in Core/Src
SOURCES=$(find Core/Src -name "*.c")

# Run clang-tidy
# -checks=...: Select checks
# --: Separator for compiler flags
clang-tidy $SOURCES \
    -checks='-*,readability-*,performance-*,bugprone-*,-performance-no-int-to-ptr,-bugprone-reserved-identifier,-readability-magic-numbers' \
    -- $INCLUDES -DSTM32C071xx -DUSE_HAL_DRIVER
