/*
 * mode_manager.h
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#ifndef INC_MODE_MANAGER_H_
#define INC_MODE_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "device/mc3479.h"
#include "model/cli_model.h"

#define FAKE_OFF_MODE_INDEX 255

typedef struct ModeManager {
    Mode currentMode; // if running out of memory, consider using a pointer here that shares cliInput.mode
    uint8_t currentModeIndex;
    MC3479 *accel;
    void (*startLedTimers)();
    void (*stopLedTimers)();
} ModeManager;

void modeManagerInit(ModeManager *manager, MC3479 *accel, void (*startLedTimers)(), void (*stopLedTimers)());
void setMode(ModeManager *manager, Mode *mode, uint8_t index);
void loadMode(ModeManager *manager, uint8_t index);
void loadModeFromBuffer(ModeManager *manager, uint8_t index, char *buffer);
bool isFakeOff(ModeManager *manager);

#endif /* INC_MODE_MANAGER_H_ */
