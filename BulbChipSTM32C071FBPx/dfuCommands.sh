# elevate to sudo and use the current users path to find stm programmer
[ "$UID" -eq 0 ] || exec sudo env PATH=$PATH bash "$0" "$@"

if (
	cd ../App/configure-app-v3 && pnpm dfu
); then
	sleep 0.3
fi

STM32_Programmer_CLI -l usb

# STM CUBE IDE Build
# STM32_Programmer_CLI --connect port=USB1 mode=UR reset=HWrst -d ./Debug/BulbChipSTM32C071FBPx.elf -v -g

# Make Build
STM32_Programmer_CLI --connect port=USB1 mode=UR reset=HWrst -d ./build/Debug/BulbChipSTM32C071FBPx.elf -v -g
