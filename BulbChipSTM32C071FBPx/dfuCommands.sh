# elevate to sudo and use the current users path to find stm programmer
[ "$UID" -eq 0 ] || exec sudo env PATH=$PATH bash "$0" "$@"

STM32_Programmer_CLI -l usb
STM32_Programmer_CLI --connect port=USB1 mode=UR reset=HWrst -d ./Debug/BulbChipSTM32C071FBPx.elf -v -g
