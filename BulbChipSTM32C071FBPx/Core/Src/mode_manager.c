/*
 * mode_manager.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "mode_manager.h"
#include <stdio.h>
#include <string.h>
#include "json/command_parser.h"
#include "json/json_buf.h"
#include "storage.h"

static const char *fakeOffModeJson =
    "{\"command\":\"writeMode\",\"index\":255,\"mode\":{\"name\":\"fakeOff\",\"front\":{"
    "\"pattern\":{\"type\":\"simple\",\"name\":\"off\",\"duration\":100,\"changeAt\":[{\"ms\":0,"
    "\"output\":\"low\"}]}}}}";
static const char *defaultModeJson =
    "{\"command\":\"writeMode\",\"index\":0,\"mode\":{\"name\":\"full "
    "on\",\"front\":{\"pattern\":{\"type\":\"simple\",\"name\":\"on\",\"duration\":100,"
    "\"changeAt\":[{\"ms\":0,\"output\":\"high\"}]}}}}";

bool modeManagerInit(
    ModeManager *manager,
    MC3479 *accel,
    RGBLed *caseLed,
    void (*enableTimers)(bool enable),
    void (*readBulbModeFromFlash)(uint8_t mode, char buffer[], uint32_t length),
    void (*writeBulbLedPin)(uint8_t state),
    WriteToSerial *writeToSerial) {
    if (!manager || !accel || !caseLed || !enableTimers || !readBulbModeFromFlash ||
        !writeBulbLedPin || !writeToSerial) {
        return false;
    }
    manager->accel = accel;
    manager->caseLed = caseLed;
    manager->enableTimers = enableTimers;
    manager->readBulbModeFromFlash = readBulbModeFromFlash;
    manager->writeBulbLedPin = writeBulbLedPin;
    manager->writeToSerial = writeToSerial;
    manager->currentModeIndex = 0;
    manager->shouldResetState = true;
    memset(&manager->modeState, 0, sizeof(manager->modeState));
    return true;
}

void setMode(ModeManager *manager, Mode *mode, uint8_t index) {
    manager->currentMode = *mode;
    manager->currentModeIndex = index;
    manager->shouldResetState = true;
    manager->enableTimers(true);

    if (manager->currentMode.hasAccel && manager->currentMode.accel.triggersCount > 0) {
        mc3479Enable(manager->accel);
    } else {
        mc3479Disable(manager->accel);
    }
}

static void readBulbMode(ModeManager *manager, uint8_t modeIndex) {
    if (modeIndex == FAKE_OFF_MODE_INDEX) {
        parseJson(fakeOffModeJson, PAGE_SECTOR, &cliInput);
    } else {
        manager->readBulbModeFromFlash(modeIndex, jsonBuf, PAGE_SECTOR);
        parseJson(jsonBuf, PAGE_SECTOR, &cliInput);
        if (cliInput.parsedType != parseWriteMode) {
            // fallback to default
            parseJson(defaultModeJson, PAGE_SECTOR, &cliInput);
        }
    }
}

void loadMode(ModeManager *manager, uint8_t index) {
    readBulbMode(manager, index);
    setMode(manager, &cliInput.mode, index);
}

/// @brief switch modes to fakeOff mode, an intermediate mode before the chips lock, enables
/// switching back on without holding button to get out of lock.
/// @param manager
/// @param enableLedTimers true if usb is plugged in to show charging status
void fakeOffMode(ModeManager *manager, bool enableLedTimers) {
    loadMode(manager, FAKE_OFF_MODE_INDEX);
    if (!enableLedTimers) {
        // used for fake off mode when not charging
        manager->enableTimers(false);
    }
}

bool isFakeOff(ModeManager *manager) {
    return manager->currentModeIndex == FAKE_OFF_MODE_INDEX;
}

typedef struct {
    ModeComponent *frontComp;
    ModeComponentState *frontState;
    ModeComponent *caseComp;
    ModeComponentState *caseState;
} ActiveComponents;

static void reportEquationError(const ModeManager *manager, const ModeEquationError *error) {
    if (!manager || !manager->writeToSerial || !error || !error->hasError) {
        return;
    }

    const char *path = error->path[0] != '\0' ? error->path : "unknown";
    const char *equation = error->equation[0] != '\0' ? error->equation : "unknown";
    char message[256];
    int written = snprintf(
        message,
        sizeof(message),
        "{\"error\":\"Equation compile error\",\"path\":\"%s\",\"position\":%d,"
        "\"equation\":\"%s\"}\n",
        path,
        error->errorPosition,
        equation);
    if (written < 0) {
        return;
    }
    if (written > (int)sizeof(message)) {
        written = (int)sizeof(message);
    }
    manager->writeToSerial(message, (uint32_t)written);
}

static ActiveComponents resolveActiveComponents(ModeManager *manager) {
    ActiveComponents active = {0};

    if (manager->currentMode.hasFront) {
        active.frontComp = &manager->currentMode.front;
        active.frontState = &manager->modeState.front;
    }

    if (manager->currentMode.hasCaseComp) {
        active.caseComp = &manager->currentMode.caseComp;
        active.caseState = &manager->modeState.case_comp;
    }

    if (manager->currentMode.hasAccel && manager->currentMode.accel.triggersCount > 0) {
        uint8_t triggerCount = manager->currentMode.accel.triggersCount;
        if (triggerCount > MODE_ACCEL_TRIGGERS_MAX) {
            triggerCount = MODE_ACCEL_TRIGGERS_MAX;
        }

        for (uint8_t i = 0; i < triggerCount; i++) {
            ModeAccelTrigger *trigger = &manager->currentMode.accel.triggers[i];
            if (isOverThreshold(manager->accel, trigger->threshold)) {
                ModeAccelTriggerState *triggerState = &manager->modeState.accel[i];

                if (trigger->hasFront) {
                    active.frontComp = &trigger->front;
                    active.frontState = &triggerState->front;
                }

                if (trigger->hasCaseComp) {
                    active.caseComp = &trigger->caseComp;
                    active.caseState = &triggerState->case_comp;
                }
            } else {
                break;
            }
        }
    }
    return active;
}

void modeTask(
    ModeManager *manager,
    uint32_t milliseconds,
    bool canUpdateCaseLed,
    uint8_t equationEvalIntervalMs) {
    if (manager->shouldResetState) {
        ModeEquationError equationError = {0};
        bool initOk = modeStateInitialize(
            &manager->modeState, &manager->currentMode, milliseconds, &equationError);
        manager->shouldResetState = false;
        if (!initOk) {
            reportEquationError(manager, &equationError);
        }
    }

    modeStateAdvance(&manager->modeState, &manager->currentMode, milliseconds);

    ActiveComponents active = resolveActiveComponents(manager);

    if (active.frontComp && active.frontState) {
        SimpleOutput output;
        if (modeStateGetSimpleOutput(
                active.frontState, active.frontComp, &output, equationEvalIntervalMs)) {
            if (output.type == BULB) {
                manager->writeBulbLedPin(output.data.bulb == high ? 1 : 0);
            } else {
                // TODO: RGB front component support
                manager->writeBulbLedPin(0);
            }
        }
    } else {
        manager->writeBulbLedPin(0);
    }

    // Update Case LED only if not evaluating button press, need to show shutdown/lock status
    if (canUpdateCaseLed) {
        if (active.caseComp && active.caseState) {
            SimpleOutput output;
            if (modeStateGetSimpleOutput(
                    active.caseState, active.caseComp, &output, equationEvalIntervalMs) &&
                output.type == RGB) {
                rgbShowUserColor(
                    manager->caseLed, output.data.rgb.r, output.data.rgb.g, output.data.rgb.b);
            }
        } else {
            rgbShowUserColor(manager->caseLed, 0, 0, 0);
        }
    }
}
