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

typedef struct SettingsManager {
    ChipSettings currentSettings;
    void (*readSettingsFromFlash)(char *buffer, uint32_t length);
} SettingsManager;

bool settingsManagerInit(
    SettingsManager *manager, void (*readSettingsFromFlash)(char *buffer, uint32_t length));
void settingsManagerLoad(SettingsManager *manager);
void settingsManagerLoadFromBuffer(SettingsManager *manager, char *buffer);
void settingsManagerUpdate(SettingsManager *manager, ChipSettings *newSettings);

#endif /* INC_SETTINGS_MANAGER_H_ */
