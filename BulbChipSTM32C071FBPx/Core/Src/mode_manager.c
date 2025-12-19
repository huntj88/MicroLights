/*
 * mode_manager.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "mode_manager.h"
#include "storage.h"
#include "json/command_parser.h"
#include <string.h>

static const char *fakeOffMode = "{\"command\":\"writeMode\",\"index\":255,\"mode\":{\"name\":\"fakeOff\",\"front\":{\"pattern\":{\"type\":\"simple\",\"name\":\"off\",\"duration\":100,\"changeAt\":[{\"ms\":0,\"output\":\"low\"}]}}}}";
static const char *defaultMode = "{\"command\":\"writeMode\",\"index\":0,\"mode\":{\"name\":\"full on\",\"front\":{\"pattern\":{\"type\":\"simple\",\"name\":\"on\",\"duration\":100,\"changeAt\":[{\"ms\":0,\"output\":\"high\"}]}}}}";

void modeManagerInit(ModeManager *manager, MC3479 *accel, void (*startLedTimers)(), void (*stopLedTimers)()) {
    manager->accel = accel;
    manager->startLedTimers = startLedTimers;
    manager->stopLedTimers = stopLedTimers;
    manager->currentModeIndex = 0;
}

void setMode(ModeManager *manager, Mode *mode, uint8_t index) {
    manager->currentMode = *mode;
    manager->currentModeIndex = index;

    if (manager->currentModeIndex == FAKE_OFF_MODE_INDEX) {
        manager->stopLedTimers();
    } else {
        manager->startLedTimers();
    }

    if (manager->currentMode.has_accel && manager->currentMode.accel.triggers_count > 0) {
        mc3479Enable(manager->accel);
    } else {
        mc3479Disable(manager->accel);
    }
}

void loadModeFromBuffer(ModeManager *manager, uint8_t index, char *buffer) {
    readBulbModeFromFlash(index, buffer, 1024);
    parseJson((uint8_t*)buffer, 1024, &cliInput);

    if (cliInput.parsedType != parseWriteMode) {
        // fallback to default
        parseJson((uint8_t*)defaultMode, 1024, &cliInput);
    }
}

static void readBulbMode(ModeManager *manager, uint8_t modeIndex) {
    if (modeIndex == FAKE_OFF_MODE_INDEX) {
        parseJson((uint8_t*)fakeOffMode, 1024, &cliInput);
    } else {
        char flashReadBuffer[1024];
        readBulbModeFromFlash(modeIndex, flashReadBuffer, 1024);
        parseJson((uint8_t*)flashReadBuffer, 1024, &cliInput);

        if (cliInput.parsedType != parseWriteMode) {
            // fallback to default
            parseJson((uint8_t*)defaultMode, 1024, &cliInput);
        }
    }
}

void loadMode(ModeManager *manager, uint8_t index) {
    readBulbMode(manager, index);
    setMode(manager, &cliInput.mode, index);
}

bool isFakeOff(ModeManager *manager) {
    return manager->currentModeIndex == FAKE_OFF_MODE_INDEX;
}
