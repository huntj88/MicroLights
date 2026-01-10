/*
 * settings_manager.h
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#ifndef INC_SETTINGS_MANAGER_H_
#define INC_SETTINGS_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>
#include "model/cli_model.h"
#include "model/storage.h"

#define SETTINGS_DEFAULTS_JSON_SIZE 200  // unit test ensures we update this if the size changes

typedef struct SettingsManager {
    ChipSettings currentSettings;
    ReadSavedSettings readSavedSettings;
} SettingsManager;

bool settingsManagerInit(
    SettingsManager *manager, void (*readSavedSettings)(char buffer[], size_t length));
void updateSettings(SettingsManager *manager, ChipSettings *newSettings);
int getSettingsDefaultsJson(char *buffer, size_t len);
int getSettingsResponse(SettingsManager *manager, char *buffer, size_t len);

#endif /* INC_SETTINGS_MANAGER_H_ */
