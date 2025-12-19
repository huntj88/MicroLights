/*
 * settings_manager.h
 *
 *  Created on: Dec 18, 2025
 *      Author: GitHub Copilot
 */

#ifndef INC_SETTINGS_MANAGER_H_
#define INC_SETTINGS_MANAGER_H_

#include <stdint.h>

typedef struct {
	uint8_t modeCount;
	uint8_t minutesUntilAutoOff;
	uint8_t minutesUntilLockAfterAutoOff;
} ChipSettings;

typedef struct {
    ChipSettings currentSettings;
} SettingsManager;

void settingsManagerLoad(SettingsManager *manager);
void settingsManagerLoadFromBuffer(SettingsManager *manager, char *buffer);
void settingsManagerUpdate(SettingsManager *manager, ChipSettings *newSettings);

#endif /* INC_SETTINGS_MANAGER_H_ */
