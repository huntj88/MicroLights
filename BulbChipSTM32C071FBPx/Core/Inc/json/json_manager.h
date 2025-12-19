/*
 * json_manager.h
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#ifndef INC_JSON_JSON_MANAGER_H_
#define INC_JSON_JSON_MANAGER_H_

#include <stdint.h>
#include "mode_manager.h"
#include "settings_manager.h"

void handleJson(ModeManager *modeManager, SettingsManager *settingsManager, uint8_t buf[], uint32_t count, void (*enterDFU)());

#endif /* INC_JSON_JSON_MANAGER_H_ */
