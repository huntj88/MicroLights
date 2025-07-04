/*
 * bq25180.c
 *
 *  Created on: Feb 1, 2025
 *      Author: jameshunt
 */

#include "bq25180.h"

HAL_StatusTypeDef write(BQ25180 *chargerIC, uint8_t register_pointer,
		uint8_t register_value) {

	uint8_t writeBuffer[2] = { 0 };
	writeBuffer[0] = register_pointer;
	writeBuffer[1] = register_value;

	HAL_StatusTypeDef statusTransmit = HAL_I2C_Master_Transmit(chargerIC->hi2c,
			chargerIC->devAddress, &writeBuffer, sizeof(writeBuffer), 1000);

	return statusTransmit;
}

HAL_StatusTypeDef configureRegister_IC_CTRL(BQ25180 *chargerIC) {
	uint8_t newConfig = IC_CTRL_DEFAULT;
	uint8_t tsEnabledMask = 0b10000000;
	newConfig &= ~tsEnabledMask; // disable ts current changes (no thermistor on my project?)
	return write(chargerIC, BQ25180_IC_CTRL, newConfig);
}

HAL_StatusTypeDef configureRegister_ICHG_CTRL(BQ25180 *chargerIC) {
	// enable charging = bit 7 in data sheet 0
	// 70 milliamp max charge current
	return write(chargerIC, BQ25180_ICHG_CTRL, 0b00100010);
}

HAL_StatusTypeDef configureRegister_VBAT_CTRL(BQ25180 *chargerIC) {
	// 4.3v, (3.5v) + (80 * 10mV), 80 = 0b1010000
	// return chargerIC->write(BQ25180_VBAT_CTRL, 0b01010000);

	// 4.4v, (3.5v) + (90 * 10mV), 90 = 0b1011010
	return write(chargerIC, BQ25180_VBAT_CTRL, 0b01011010);
}

HAL_StatusTypeDef configureRegister_CHARGECTRL1(BQ25180 *chargerIC) {
	// Battery Discharge Current Limit
	// 2b00 = 500mA

	// Battery Undervoltage LockOut Falling Threshold.
	// 3b000 = 3.0V

	// Mask Charging Status Interrupt = OFF 1b1
	// Mask ILIM Fault Interrupt = OFF 1b1
	// Mask VINDPM and VDPPM Interrupt = OFF 1b1, TODO: turn back on?, 1b0

	return write(chargerIC, BQ25180_CHARGECTRL1, 0b00000111);
}

HAL_StatusTypeDef configureRegister_MASK_ID(BQ25180 *chargerIC) {
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

	return write(chargerIC, BQ25180_MASK_ID, 0b00000000);
}

void configureChargerIC(BQ25180 *chargerIC) {
	HAL_StatusTypeDef status[] = { 5 };
	status[0] = configureRegister_IC_CTRL(chargerIC);
	status[1] = configureRegister_ICHG_CTRL(chargerIC);
	status[2] = configureRegister_VBAT_CTRL(chargerIC);
	status[3] = configureRegister_CHARGECTRL1(chargerIC);
	status[4] = configureRegister_MASK_ID(chargerIC);
}

void print(UART_HandleTypeDef *huart, char *stringToPrint) {
	HAL_UART_Transmit(huart, stringToPrint, strlen(stringToPrint), 100);
}

void printBinary(UART_HandleTypeDef *huart, uint8_t num) {
	char buffer[8] = { 0 };
	char *bufferPtr = &buffer;

	sprintf(bufferPtr + 0, "%d", (num & 0b10000000) > 0 ? 1 : 0);
	sprintf(bufferPtr + 1, "%d", (num & 0b01000000) > 0 ? 1 : 0);
	sprintf(bufferPtr + 2, "%d", (num & 0b00100000) > 0 ? 1 : 0);
	sprintf(bufferPtr + 3, "%d", (num & 0b00010000) > 0 ? 1 : 0);
	sprintf(bufferPtr + 4, "%d", (num & 0b00001000) > 0 ? 1 : 0);
	sprintf(bufferPtr + 5, "%d", (num & 0b00000100) > 0 ? 1 : 0);
	sprintf(bufferPtr + 6, "%d", (num & 0b00000010) > 0 ? 1 : 0);
	sprintf(bufferPtr + 7, "%d", (num & 0b00000001) > 0 ? 1 : 0);

	HAL_UART_Transmit(huart, &buffer, sizeof(buffer), 100);
}

void readRegisters(BQ25180 *chargerIC) {
	char newline[2] = "\n";
	HAL_UART_Transmit(chargerIC->huart, &newline, sizeof(newline), 100);

	readRegister(chargerIC, BQ25180_STAT0, "STAT0");
	readRegister(chargerIC, BQ25180_STAT1, "STAT1");
	readRegister(chargerIC, BQ25180_FLAG0, "FLAG0");
	readRegister(chargerIC, BQ25180_VBAT_CTRL, "VBAT_CTRL");
	readRegister(chargerIC, BQ25180_ICHG_CTRL, "ICHG_CTRL");
	readRegister(chargerIC, BQ25180_CHARGECTRL0, "CHARGECTRL0");
	readRegister(chargerIC, BQ25180_CHARGECTRL1, "CHARGECTRL1");
	readRegister(chargerIC, BQ25180_IC_CTRL, "IC_CTRL");
	readRegister(chargerIC, BQ25180_TMR_ILIM, "TMR_ILIM");
	readRegister(chargerIC, BQ25180_SHIP_RST, "SHIP_RST");
	readRegister(chargerIC, BQ25180_SYS_REG, "SYS_REG");
	readRegister(chargerIC, BQ25180_TS_CONTROL, "TS_CONTROL");
	readRegister(chargerIC, BQ25180_MASK_ID, "MASK_ID");
}

void readRegister(BQ25180 *chargerIC, uint8_t reg, char *label) {
	uint8_t receive_buffer[1] = { 0 };

	HAL_StatusTypeDef statusTransmit = HAL_I2C_Master_Transmit(chargerIC->hi2c,
			chargerIC->devAddress, &reg, 1, 1000);

	HAL_StatusTypeDef statusReceive = HAL_I2C_Master_Receive(chargerIC->hi2c,
			chargerIC->devAddress, &receive_buffer, 1, 1000);

	print(chargerIC->huart, "register");
	print(chargerIC->huart, " ");
	print(chargerIC->huart, label);
	print(chargerIC->huart, ": ");

	printBinary(chargerIC->huart, *receive_buffer);

	char newline[2] = "\n";
	HAL_UART_Transmit(chargerIC->huart, &newline, sizeof(newline), 100);
}
