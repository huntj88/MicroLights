/*
 * bq25180.c
 *
 *  Created on: Feb 1, 2025
 *      Author: jameshunt
 */

#include "bq25180.h"

HAL_StatusTypeDef write(I2C_HandleTypeDef *hi2c, uint8_t register_pointer,
		uint8_t register_value) {

	uint8_t writeBuffer[2] = { 0 };
	writeBuffer[0] = register_pointer;
	writeBuffer[1] = register_value;

	HAL_StatusTypeDef statusTransmit = HAL_I2C_Master_Transmit(hi2c,
			(0x6A << 1), &writeBuffer, sizeof(writeBuffer), 1000);

	return statusTransmit;
}

HAL_StatusTypeDef configureRegister_IC_CTRL(I2C_HandleTypeDef *hi2c) {
	uint8_t newConfig = IC_CTRL_DEFAULT;
	uint8_t tsEnabledMask = 0b10000000;
	newConfig &= ~tsEnabledMask; // disable ts current changes (no thermistor on my project?)
	return write(hi2c, BQ25180_IC_CTRL, newConfig);
}

HAL_StatusTypeDef configureRegister_ICHG_CTRL(I2C_HandleTypeDef *hi2c) {
	// enable charging = bit 7 in data sheet 0
	// 70 milliamp max charge current
	return write(hi2c, BQ25180_ICHG_CTRL, 0b00100010);
}

HAL_StatusTypeDef configureRegister_VBAT_CTRL(I2C_HandleTypeDef *hi2c) {
	// 4.3v, (3.5v) + (80 * 10mV), 80 = 0b1010000
	// return chargerIC->write(BQ25180_VBAT_CTRL, 0b01010000);

	// 4.4v, (3.5v) + (90 * 10mV), 90 = 0b1011010
	return write(hi2c, BQ25180_VBAT_CTRL, 0b01011010);
}

HAL_StatusTypeDef configureRegister_CHARGECTRL1(I2C_HandleTypeDef *hi2c) {
	// Battery Discharge Current Limit
	// 2b00 = 500mA

	// Battery Undervoltage LockOut Falling Threshold.
	// 3b000 = 3.0V

	// Mask Charging Status Interrupt = OFF 1b1
	// Mask ILIM Fault Interrupt = OFF 1b1
	// Mask VINDPM and VDPPM Interrupt = OFF 1b1, TODO: turn back on?, 1b0

	return write(hi2c, BQ25180_CHARGECTRL1, 0b00000111);
}

HAL_StatusTypeDef configureRegister_MASK_ID(I2C_HandleTypeDef *hi2c) {
	// TS_INT_MASK: Mask or enable the TS interrupt.
	//   1b0: Enable TS interrupt
	//   1b1: Mask TS interrupt

	// TREG_INT_MASK: Mask or enable the TREG interrupt.
	//   1b0: Enable TREG interrupt
	//   1b1: Mask TREG interrupt

	// BAT_INT_MASK: Mask or enable BATOCP and BUVLO interrupts.
	//   1b0: Enable BATOCP and BUVLO interrupts
	//   1b1: Mask BATOCP and BUVLO interrupts

	// PG_INT_MASK: Mask or enable PG and VINOVP interrupts.
	//   1b0: Enable PG and VINOVP interrupts
	//   1b1: Mask PG and VINOVP interrupts

	// Device_ID: A 4-bit field indicating the device ID.
	//   4b0000: Device ID for the BQ25180.

	return write(hi2c, BQ25180_MASK_ID, 0b00000000);
}

void configureChargerIC(I2C_HandleTypeDef *hi2c) {
	HAL_StatusTypeDef status[] = { 5 };
	status[0] = configureRegister_IC_CTRL(hi2c);
	status[1] = configureRegister_ICHG_CTRL(hi2c);
	status[2] = configureRegister_VBAT_CTRL(hi2c);
	status[3] = configureRegister_CHARGECTRL1(hi2c);
	status[4] = configureRegister_MASK_ID(hi2c);
}

void printLine(UART_HandleTypeDef *huart, char *stringToPrint) {
	HAL_UART_Transmit(huart, stringToPrint, strlen(stringToPrint), 100);

	char newline[2] = "\n";
	HAL_UART_Transmit(huart, &newline, sizeof(newline), 100);
}

void readRegister_STAT0(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart, uint16_t devAddress) {
	uint8_t receive_buffer[1] = { 0 };

	HAL_StatusTypeDef statusTransmit = HAL_I2C_Master_Transmit(hi2c,
			devAddress, BQ25180_STAT0, 1, 1000);

	HAL_StatusTypeDef statusReceive = HAL_I2C_Master_Receive(hi2c, devAddress,
			&receive_buffer, 1, 1000);


	printRegister_STAT0(huart, receive_buffer);
}

void printRegister_STAT0(UART_HandleTypeDef *huart, uint8_t flags) {
	uint8_t TS_OPEN_STAT = flags & 0b10000000;
	uint8_t CHG_STAT_1 = flags & 0b01000000;
	uint8_t CHG_STAT_0 = flags & 0b00100000;
	uint8_t ILIM_ACTIVE_STAT = flags & 0b00010000;
	uint8_t VDPPM_ACTIVE_STAT = flags & 0b00001000;
	uint8_t VINDPM_ACTIVE_STAT = flags & 0b00000100;
	uint8_t THERMREG_ACTIVE_STAT = flags & 0b00000010;
	uint8_t VIN_PGOOD_STAT = flags & 0b00000001;

	// Description of each bit field in STAT0 register

	// TS_OPEN_STAT (Bit 7)
	printLine(huart, "Bit 7 TS_OPEN_STAT");
	if (TS_OPEN_STAT) {
		printLine(huart, " 1 TS is open");
	} else {
		printLine(huart, " 0 TS is connected");
	}

	// CHG_STAT (Bits 6-5)
	printLine(huart, "Bits 6-5 CHG_STAT");
	if (CHG_STAT_1) {
		if (CHG_STAT_0) {
			printLine(huart, " 11 Charge termination done");
		} else {
			printLine(huart, " 10 Fast charging");
		}
	} else {
		if (CHG_STAT_0) {
			printLine(huart, " 01 Pre-charge in progress");
		} else {
			printLine(huart, " 00 Not Charging");
		}
	}

	// ILIM_ACTIVE_STAT (Bit 4)
	printLine(huart, "Bit 4 ILIM_ACTIVE_STAT");
	if (ILIM_ACTIVE_STAT) {
		printLine(huart, " 1 Input Current Limit Active");
	} else {
		printLine(huart, " 0 Input Current Limit Not Active");
	}

	// VDPPM_ACTIVE_STAT (Bit 3)
	printLine(huart, "Bit 3 VDPPM_ACTIVE_STAT");
	if (VDPPM_ACTIVE_STAT) {
		printLine(huart, " 1 VDPPM Mode Active");
	} else {
		printLine(huart, " 0 VDPPM Mode Not Active");
	}

	// VINDPM_ACTIVE_STAT (Bit 2)
	printLine(huart, "Bit 2 VINDPM_ACTIVE_STAT");
	if (VINDPM_ACTIVE_STAT) {
		printLine(huart, " 1 VINDPM Mode Active");
	} else {
		printLine(huart, " 0 VINDPM Mode Not Active");
	}

	// THERMREG_ACTIVE_STAT (Bit 1)
	printLine(huart, "Bit 1 THERMREG_ACTIVE_STAT");
	if (THERMREG_ACTIVE_STAT) {
		printLine(huart, " 1 Thermal Regulation Active");
	} else {
		printLine(huart, " 0 Thermal Regulation Not Active");
	}

	// VIN_PGOOD_STAT (Bit 0)
	printLine(huart, "Bit 0 VIN_PGOOD_STAT");
	if (VIN_PGOOD_STAT) {
		printLine(huart, " 1 VIN Power Good");
	} else {
		printLine(huart, " 0 VIN Power Not Good");
	}
}
