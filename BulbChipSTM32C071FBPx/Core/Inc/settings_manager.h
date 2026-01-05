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

#define SETTINGS_DEFAULTS_JSON_SIZE 131  // unit test ensures we update this if the size changes

typedef struct SettingsManager {
    ChipSettings currentSettings;
    void (*readSettingsFromFlash)(char buffer[], uint32_t length);
} SettingsManager;

bool settingsManagerInit(
    SettingsManager *manager, void (*readSettingsFromFlash)(char buffer[], uint32_t length));
void updateSettings(SettingsManager *manager, ChipSettings *newSettings);
int getSettingsDefaultsJson(char *buffer, uint32_t len);
int getSettingsResponse(SettingsManager *manager, char *buffer, uint32_t len);

#endif /* INC_SETTINGS_MANAGER_H_ */
