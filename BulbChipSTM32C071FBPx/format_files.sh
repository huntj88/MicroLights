#!/bin/bash
find Core Tests \( -name '*.c' -o -name '*.h' \) -not -name 'stm32c0xx_*' -not -name 'system_stm32c0xx.c' -not -name 'syscalls.c' -not -name 'sysmem.c' | xargs clang-format -i
