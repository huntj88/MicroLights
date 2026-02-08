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
#include "model/log.h"
#include "model/mode_state.h"
#include "model/storage.h"

#define FAKE_OFF_MODE_INDEX 255

// TODO: split deps into separate struct like chipState?
typedef struct ModeManager {
    Mode currentMode;  // if running out of memory, consider using a pointer here that shares
                       // cliInput.mode
    uint8_t currentModeIndex;
    MC3479 *accel;
    RGBLed *caseLed;
    RGBLed *frontLed;
    ReadSavedMode readSavedMode;
    void (*writeBulbLedPin)(uint8_t state);
    Log log;
    ModeState modeState;
    bool shouldResetState;
} ModeManager;

typedef struct ModeOutputs {
    bool frontValid;
    bool caseValid;
    SimpleOutputType frontType;
} ModeOutputs;

bool modeManagerInit(
    ModeManager *manager,
    MC3479 *accel,
    RGBLed *caseLed,
    RGBLed *frontLed,
    ReadSavedMode readSavedMode,
    void (*writeBulbLedPin)(uint8_t state),
    Log log);
void setMode(ModeManager *manager, Mode *mode, uint8_t index);
void loadMode(ModeManager *manager, uint8_t index);

void fakeOffMode(ModeManager *manager);
bool isFakeOff(ModeManager *manager);
ModeOutputs modeTask(
    ModeManager *manager,
    uint32_t milliseconds,
    bool canUpdateCaseLed,
    uint8_t equationEvalIntervalMs);

#endif /* INC_MODE_MANAGER_H_ */
