/*
 * json_manager.c
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#include "json/json_manager.h"
#include "json/command_parser.h"
#include "chip_state.h"
#include "storage.h"
#include <string.h>

void handleJson(ModeManager *modeManager, SettingsManager *settingsManager, uint8_t buf[], uint32_t count, void (*enterDFU)()) {
	parseJson(buf, count, &cliInput);

	switch (cliInput.parsedType) {
	case parseError: {
		// TODO return errors from mode parser
		char error[] = "{\"error\":\"unable to parse json\"}\n";
		chip_state_write_serial(error);
		break;
	}
	case parseWriteMode: {
		if (strcmp(cliInput.mode.name, "transientTest") == 0) {
			// do not write to flash for transient test
			setMode(modeManager, &cliInput.mode, cliInput.modeIndex);
		} else {
			writeBulbModeToFlash(cliInput.modeIndex, buf, cliInput.jsonLength);
			setMode(modeManager, &cliInput.mode, cliInput.modeIndex);
		}
		break;
	}
	case parseReadMode: {
		char flashReadBuffer[1024];
		loadModeFromBuffer(modeManager, cliInput.modeIndex, flashReadBuffer);
		uint16_t len = strlen(flashReadBuffer);
		flashReadBuffer[len] = '\n';
		flashReadBuffer[len + 1] = '\0';
		chip_state_write_serial(flashReadBuffer);
		break;
	}
	case parseWriteSettings: {
		ChipSettings settings = cliInput.settings;
		writeSettingsToFlash(buf, cliInput.jsonLength);
		settingsManagerUpdate(settingsManager, &settings);
		break;
	}
	case parseReadSettings: {
		char flashReadBuffer[1024];
		settingsManagerLoadFromBuffer(settingsManager, flashReadBuffer);
		uint16_t len = strlen(flashReadBuffer);
		flashReadBuffer[len] = '\n';
		flashReadBuffer[len + 1] = '\0';
		chip_state_write_serial(flashReadBuffer);
		break;
	}
	case parseDfu: {
		enterDFU();
		break;
	}}

	if (cliInput.parsedType != parseError) {
		chip_state_show_success();
	}
}
