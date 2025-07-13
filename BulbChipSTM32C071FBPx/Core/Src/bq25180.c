/*
 * bq25180.c
 *
 *  Created on: Feb 1, 2025
 *      Author: jameshunt
 */

#include "bq25180.h"
#include "rgb.h"

#define NOT_CONNECTED 0
#define NOT_CHARGING 1
#define CONSTANT_CURRENT_CHARGING 2
#define CONSTANT_VOLTAGE_CHARGING 3
#define DONE_CHARGING 4

static uint16_t tickCount = 0;
static uint8_t readNow = 0;

void checkInterrupt() {
	readNow = 1;
}

uint8_t getChargingState(BQ25180 *chargerIC) {
	uint8_t regResult = chargerIC->readRegister(chargerIC, BQ25180_STAT0);

	if ((regResult & 0b01000000) > 0) {
		if ((regResult & 0b00100000) > 0) {
			return DONE_CHARGING;
		} else {
			return CONSTANT_VOLTAGE_CHARGING;
		}
	} else if ((regResult & 0b00100000) > 0) {
		return CONSTANT_CURRENT_CHARGING;
	}

	// check if plugged in
	if ((regResult & 0b00000001) > 0) {
		return NOT_CHARGING;
	} else {
		return NOT_CONNECTED;
	}
}

void showChargingState(BQ25180 *chargerIC) {
	uint8_t state = getChargingState(chargerIC);
	if (state == NOT_CONNECTED) {
		showColor(0, 0, 0);
	} else if (state == NOT_CHARGING) {
		showColor(40, 0, 0);
	} else if (state == CONSTANT_CURRENT_CHARGING) {
		showColor(30, 10, 0);
	} else if (state == CONSTANT_VOLTAGE_CHARGING) {
		showColor(10, 30, 0);
	} else if (state == DONE_CHARGING) {
		showColor(0, 40, 0);
	}
}

void charger_task(BQ25180 *chargerIC) {
	if (tickCount % 1024 == 0) {
		configureChargerIC(chargerIC);
		readRegisters(chargerIC);
		showChargingState(chargerIC);
	}

	if (readNow) {
		readNow = 0;
		showChargingState(chargerIC);
		readRegisters(chargerIC);
	}

	tickCount++;
}

void configureRegister_IC_CTRL(BQ25180 *chargerIC) {
	uint8_t newConfig = IC_CTRL_DEFAULT;
	uint8_t tsEnabledMask = 0b10000000;
	newConfig &= ~tsEnabledMask; // disable ts current changes (no thermistor on my project?)
	return chargerIC->writeRegister(chargerIC, BQ25180_IC_CTRL, newConfig);
}

void configureRegister_ICHG_CTRL(BQ25180 *chargerIC) {
	// enable charging = bit 7 in data sheet 0
	// 70 milliamp max charge current
	return chargerIC->writeRegister(chargerIC, BQ25180_ICHG_CTRL, 0b00100010);
}

void configureRegister_VBAT_CTRL(BQ25180 *chargerIC) {
	// 4.3v, (3.5v) + (80 * 10mV), 80 = 0b1010000
	// return chargerIC->write(BQ25180_VBAT_CTRL, 0b01010000);

	// 4.4v, (3.5v) + (90 * 10mV), 90 = 0b1011010
	return chargerIC->writeRegister(chargerIC, BQ25180_VBAT_CTRL, 0b01011010);
}

void configureRegister_CHARGECTRL1(BQ25180 *chargerIC) {
	// Battery Discharge Current Limit
	// 2b00 = 500mA

	// Battery Undervoltage LockOut Falling Threshold.
	// 3b000 = 3.0V

	// Mask Charging Status Interrupt = ON 1b0
	// Mask ILIM Fault Interrupt = OFF 1b1
	// Mask VINDPM and VDPPM Interrupt = OFF 1b1, TODO: turn back on?, 1b0

	return chargerIC->writeRegister(chargerIC, BQ25180_CHARGECTRL1, 0b00000011);
}

void configureRegister_MASK_ID(BQ25180 *chargerIC) {
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

	return chargerIC->writeRegister(chargerIC, BQ25180_MASK_ID, 0b00000000);
}

void configureChargerIC(BQ25180 *chargerIC) {
	configureRegister_IC_CTRL(chargerIC);
	configureRegister_ICHG_CTRL(chargerIC);
	configureRegister_VBAT_CTRL(chargerIC);
	configureRegister_CHARGECTRL1(chargerIC);
	configureRegister_MASK_ID(chargerIC);
}

void print(BQ25180 *chargerIC, char *stringToPrint) {
	chargerIC->writeToUsbSerial(0, stringToPrint, strlen(stringToPrint));
}

void printBinary(BQ25180 *chargerIC, uint8_t num) {
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

	chargerIC->writeToUsbSerial(0, buffer, sizeof(buffer));
}

void readRegisters(BQ25180 *chargerIC) {
	char newLine[1] = "\n";
	chargerIC->writeToUsbSerial(0, newLine, sizeof(newLine));


	printRegister(chargerIC, BQ25180_STAT0, "STAT0");
	printRegister(chargerIC, BQ25180_STAT1, "STAT1");
	printRegister(chargerIC, BQ25180_FLAG0, "FLAG0");
	printRegister(chargerIC, BQ25180_VBAT_CTRL, "VBAT_CTRL");
	printRegister(chargerIC, BQ25180_ICHG_CTRL, "ICHG_CTRL");
	printRegister(chargerIC, BQ25180_CHARGECTRL0, "CHARGECTRL0");
	printRegister(chargerIC, BQ25180_CHARGECTRL1, "CHARGECTRL1");
	printRegister(chargerIC, BQ25180_IC_CTRL, "IC_CTRL");
	printRegister(chargerIC, BQ25180_TMR_ILIM, "TMR_ILIM");
	printRegister(chargerIC, BQ25180_SHIP_RST, "SHIP_RST");
	printRegister(chargerIC, BQ25180_SYS_REG, "SYS_REG");
	printRegister(chargerIC, BQ25180_TS_CONTROL, "TS_CONTROL");
	printRegister(chargerIC, BQ25180_MASK_ID, "MASK_ID");
}

void printRegister(BQ25180 *chargerIC, uint8_t reg, char *label) {
	uint8_t receive_buffer[1] = { 0 };

	uint8_t regValue = chargerIC->readRegister(chargerIC, reg);

	print(chargerIC, "register");
	print(chargerIC, " ");
	print(chargerIC, label);
	print(chargerIC, ": ");

	printBinary(chargerIC, regValue);

	char newLine[1] = "\n";
	chargerIC->writeToUsbSerial(0, newLine, sizeof(newLine));
}

// power must be unplugged to enter ship mode.
void enableShipMode(BQ25180 *chargerIC) {
	// REG_RST - Software Reset
	// 1b0 = Do nothing
	// 1b1 = Software Reset

	// EN_RST_SHIP_1 - Shipmode Enable and Hardware Reset, once bits set, power must be unplugged to enter ship mode.
	// 2b00 = Do nothing
	// 2b01 = Enable shutdown mode with wake on adapter insert only
	// 2b10 = Enable shipmode with wake on button press or adapter insert
	// 2b11 = Hardware Reset

	// PB_LPRESS_ACTION_1 - Pushbutton long press action
	// 2b00 = Do nothing
	// 2b01 = Hardware Reset
	// 2b10 = Enable shipmode
	// 2b11 = Enable shutdown mode

	// WAKE1_TMR - Wake 1 Timer Set
	// 1b0 = 300ms
	// 1b1 = 1s

	// WAKE2_TMR - Wake 2 Timer Set
	// 1b0 = 2s
	// 1b1 = 3s

	// EN_PUSH - Enable Push Button and Reset Function on Battery Only
	// 1b0 = Disable
	// 1b1 = Enable

	return chargerIC->writeRegister(chargerIC, BQ25180_SHIP_RST, 0b01000001);
	//     write(chargerIC, BQ25180_SHIP_RST, 0b10000001); // software reset
}
