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

	HAL_StatusTypeDef statusTransmit = HAL_I2C_Master_Transmit(hi2c, (0x6A << 1), &writeBuffer, sizeof(writeBuffer), 1000);

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
	HAL_StatusTypeDef status[] = {5};
	status[0] = configureRegister_IC_CTRL(hi2c);
	status[1] = configureRegister_ICHG_CTRL(hi2c);
	status[2] = configureRegister_VBAT_CTRL(hi2c);
	status[3] = configureRegister_CHARGECTRL1(hi2c);
	status[4] = configureRegister_MASK_ID(hi2c);
}
