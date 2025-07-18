# execute this script with
# sudo env PATH=$PATH bash dfuCommands.sh

STM32_Programmer_CLI -l usb
STM32_Programmer_CLI --connect port=USB1 mode=UR reset=HWrst -d ./Debug/BulbChipSTM32C071FBPx.elf -v -g
