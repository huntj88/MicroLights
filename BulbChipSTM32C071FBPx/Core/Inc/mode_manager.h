/*
 * mode_manager.h
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#ifndef INC_MODE_MANAGER_H_
#define INC_MODE_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>
#include "device/mc3479.h"
#include "device/rgb_led.h"
#include "model/cli_model.h"
#include "model/mode_state.h"

#define FAKE_OFF_MODE_INDEX 255

typedef struct ModeManager {
    Mode currentMode;  // if running out of memory, consider using a pointer here that shares
                       // cliInput.mode
    uint8_t currentModeIndex;
    MC3479 *accel;
    RGBLed *caseLed;
    void (*enableTimers)(bool enable);
    void (*readBulbModeFromFlash)(uint8_t mode, char *buffer, uint32_t length);
    void (*writeBulbLedPin)(uint8_t state);
    ModeState modeState;
    bool shouldResetState;
} ModeManager;

bool modeManagerInit(
    ModeManager *manager,
    MC3479 *accel,
    RGBLed *caseLed,
    void (*enableTimers)(bool enable),
    void (*readBulbModeFromFlash)(uint8_t mode, char *buffer, uint32_t length),
    void (*writeBulbLedPin)(uint8_t state));
void setMode(ModeManager *manager, Mode *mode, uint8_t index);
void loadMode(ModeManager *manager, uint8_t index);

// TODO: rename loadModeWithBuffer?
void loadModeFromBuffer(ModeManager *manager, uint8_t index, char *buffer);
void fakeOffMode(ModeManager *manager, bool enableLedTimers);
bool isFakeOff(ModeManager *manager);
void modeTask(ModeManager *manager, uint32_t ms, bool canUpdateCaseLed);

#endif /* INC_MODE_MANAGER_H_ */
