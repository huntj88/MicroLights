/*
 * mode_manager.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "microlight/mode_manager.h"
#include <stdio.h>
#include <string.h>
#include "microlight/json/command_parser.h"
#include "microlight/json/json_buf.h"

static const char *fakeOffModeJson =
    "{\"command\":\"writeMode\",\"index\":255,\"mode\":{\"name\":\"fakeOff\",\"front\":{"
    "\"pattern\":{\"type\":\"simple\",\"name\":\"off\",\"duration\":100,\"changeAt\":[{\"ms\":0,"
    "\"output\":\"low\"}]}}}}";
static const char *defaultModeJson =
    "{\"command\":\"writeMode\",\"index\":0,\"mode\":{\"name\":\"full "
    "on\",\"front\":{\"pattern\":{\"type\":\"simple\",\"name\":\"on\",\"duration\":100,"
    "\"changeAt\":[{\"ms\":0,\"output\":\"high\"}]}}}}";

static void disableFrontOutputs(ModeManager *manager, ModeOutputs *outputs);
static void disableCaseOutputs(ModeManager *manager, ModeOutputs *outputs);
static void handleFrontOutput(
    ModeManager *manager,
    ModeComponentState *state,
    ModeComponent *component,
    ModeOutputs *outputs,
    uint8_t equationEvalIntervalMs);
static void handleCaseOutput(
    ModeManager *manager,
    ModeComponentState *state,
    ModeComponent *component,
    ModeOutputs *outputs,
    uint8_t equationEvalIntervalMs);

bool modeManagerInit(
    ModeManager *manager,
    MC3479 *accel,
    RGBLed *caseLed,
    RGBLed *frontLed,
    ReadSavedMode readSavedMode,
    void (*writeBulbLedPin)(uint8_t state),
    Log log) {
    if (!manager || !accel || !caseLed || !frontLed || !readSavedMode || !writeBulbLedPin || !log ||
        caseLed == frontLed) {
        return false;
    }
    manager->accel = accel;
    manager->caseLed = caseLed;
    manager->frontLed = frontLed;
    manager->readSavedMode = readSavedMode;
    manager->writeBulbLedPin = writeBulbLedPin;
    manager->log = log;
    manager->currentModeIndex = 0;
    manager->shouldResetState = true;
    memset(&manager->modeState, 0, sizeof(manager->modeState));
    return true;
}

void setMode(ModeManager *manager, Mode *mode, uint8_t index) {
    manager->currentMode = *mode;
    manager->currentModeIndex = index;
    manager->shouldResetState = true;

    if (manager->currentMode.hasAccel && manager->currentMode.accel.triggersCount > 0) {
        mc3479Enable(manager->accel);
    } else {
        mc3479Disable(manager->accel);
    }
}

static void readBulbMode(ModeManager *manager, uint8_t modeIndex) {
    if (modeIndex == FAKE_OFF_MODE_INDEX) {
        parseJson(fakeOffModeJson, sharedJsonIOBufferLength, &cliInput);
    } else {
        manager->readSavedMode(modeIndex, sharedJsonIOBuffer, sharedJsonIOBufferLength);
        parseJson(sharedJsonIOBuffer, sharedJsonIOBufferLength, &cliInput);
        if (cliInput.parsedType != parseWriteMode) {
            char msg[64];
            int len = snprintf(
                msg, sizeof(msg), "{\"error\":\"corrupt saved mode\",\"mode\":%u}\n", modeIndex);
            if (len > 0) {
                manager->log(msg, (size_t)len);
            }
            // TODO: log entire mode JSON for debugging?

            // Fall back to default mode if saved mode is corrupt
            parseJson(defaultModeJson, sharedJsonIOBufferLength, &cliInput);
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
void fakeOffMode(ModeManager *manager) {
    loadMode(manager, FAKE_OFF_MODE_INDEX);
    disableFrontOutputs(manager, NULL);
}

bool isFakeOff(ModeManager *manager) {
    return manager->currentModeIndex == FAKE_OFF_MODE_INDEX;
}

static void disableFrontOutputs(ModeManager *manager, ModeOutputs *outputs) {
    if (!manager) {
        return;
    }
    // Legacy: ensure bulb GPIO is forced low when PWM front output is not used.
    manager->writeBulbLedPin(0);
    rgbShowUserColor(manager->frontLed, 0, 0, 0);
    if (outputs) {
        outputs->frontValid = false;
    }
}

static void disableCaseOutputs(ModeManager *manager, ModeOutputs *outputs) {
    if (!manager) {
        return;
    }
    rgbShowUserColor(manager->caseLed, 0, 0, 0);
    if (outputs) {
        outputs->caseValid = false;
    }
}

static void handleFrontOutput(
    ModeManager *manager,
    ModeComponentState *state,
    ModeComponent *component,
    ModeOutputs *outputs,
    uint8_t equationEvalIntervalMs) {
    if (!manager || !state || !component) {
        disableFrontOutputs(manager, outputs);
        return;
    }

    SimpleOutput output;
    if (!modeStateGetSimpleOutput(state, component, &output, equationEvalIntervalMs)) {
        disableFrontOutputs(manager, outputs);
        return;
    }

    if (output.type == BULB) {
        if (outputs) {
            outputs->frontValid = true;
            outputs->frontType = BULB;
        }
        manager->writeBulbLedPin(output.data.bulb == high ? 1 : 0);
        return;
    }

    if (output.type == RGB) {
        if (outputs) {
            outputs->frontValid = true;
            outputs->frontType = RGB;
        }
        // Legacy: ensure bulb GPIO is forced low when RGB is active.
        manager->writeBulbLedPin(0);
        rgbShowUserColor(
            manager->frontLed, output.data.rgb.r, output.data.rgb.g, output.data.rgb.b);
        return;
    }

    disableFrontOutputs(manager, outputs);
}

static void handleCaseOutput(
    ModeManager *manager,
    ModeComponentState *state,
    ModeComponent *component,
    ModeOutputs *outputs,
    uint8_t equationEvalIntervalMs) {
    if (!manager || !state || !component) {
        disableCaseOutputs(manager, outputs);
        return;
    }

    SimpleOutput output;
    if (!modeStateGetSimpleOutput(state, component, &output, equationEvalIntervalMs) ||
        output.type != RGB) {
        disableCaseOutputs(manager, outputs);
        return;
    }

    if (outputs) {
        outputs->caseValid = true;
    }
    rgbShowUserColor(manager->caseLed, output.data.rgb.r, output.data.rgb.g, output.data.rgb.b);
}

typedef struct {
    ModeComponent *frontComp;
    ModeComponentState *frontState;
    ModeComponent *caseComp;
    ModeComponentState *caseState;
} ActiveComponents;

static void reportEquationError(const ModeManager *manager, const ModeEquationError *error) {
    if (!manager || !manager->log || !error || !error->hasError) {
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
    manager->log(message, (size_t)written);
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

ModeOutputs modeTask(
    ModeManager *manager,
    uint32_t milliseconds,
    bool canUpdateCaseLed,
    uint8_t equationEvalIntervalMs) {
    ModeOutputs outputs = {
        .frontValid = false,
        .caseValid = false,
        .frontType = BULB,
    };
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

    handleFrontOutput(
        manager, active.frontState, active.frontComp, &outputs, equationEvalIntervalMs);

    // Update Case LED only if not evaluating button press, need to show shutdown/lock status
    if (canUpdateCaseLed) {
        handleCaseOutput(
            manager, active.caseState, active.caseComp, &outputs, equationEvalIntervalMs);
    }

    return outputs;
}
