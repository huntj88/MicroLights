#include "microlight/microlight.h"
#include <stdio.h>
#include <string.h>
#include "microlight/i2c_log_decorate.h"
#include "microlight/json/json_buf.h"

static BQ25180 chargerIC;
static Button button;
static MC3479 accel;
static RGBLed caseLed;
static ModeManager modeManager;
static SettingsManager settingsManager;
static USBManager usbManager;

static void internalLog(const char *buf, size_t count) {
    if (usbManager.usbWriteToSerial != NULL) {
        usbManager.usbWriteToSerial(buf, count);
    }
}

// Wrap I2C dependencies to handle logging internal to this file
static I2CWriteRegisterChecked *rawI2cWrite = NULL;
static I2CReadRegisters *rawI2cReadRegs = NULL;

static void internalI2cWriteRegister(uint8_t devAddress, uint8_t reg, uint8_t value) {
    i2cDecoratedWrite(
        devAddress,
        reg,
        value,
        rawI2cWrite,
        &settingsManager.currentSettings.enableI2cFailureReporting,
        internalLog);
}

static bool internalI2cReadRegisters(
    uint8_t devAddress, uint8_t startReg, uint8_t *buf, size_t len) {
    return i2cDecoratedReadRegisters(
        devAddress,
        startReg,
        buf,
        len,
        rawI2cReadRegs,
        &settingsManager.currentSettings.enableI2cFailureReporting,
        internalLog);
}

void configureMicroLight(MicroLightDependencies *deps) {
    if (!initSharedJsonIOBuffer(deps->jsonBuffer, deps->jsonBufferSize)) {
        deps->errorHandler();
    }

    if (deps->i2cWriteRegister == NULL || deps->i2cReadRegisters == NULL) {
        deps->errorHandler();
    }
    rawI2cWrite = deps->i2cWriteRegister;
    rawI2cReadRegs = deps->i2cReadRegisters;

    if (!rgbInit(&caseLed, deps->writeRgbPwmCaseLed, (uint16_t)deps->rgbTimerPeriod)) {
        deps->errorHandler();
    }

    if (!buttonInit(&button, deps->readButtonPin, deps->enableTimers, &caseLed)) {
        deps->errorHandler();
    }

    if (!mc3479Init(
            &accel, internalI2cReadRegisters, internalI2cWriteRegister, MC3479_I2CADDR_DEFAULT)) {
        deps->errorHandler();
    }

    if (!modeManagerInit(
            &modeManager,
            &accel,
            &caseLed,
            deps->enableTimers,
            deps->readSavedMode,
            deps->writeBulbLed,
            internalLog)) {
        deps->errorHandler();
    }
    if (!settingsManagerInit(&settingsManager, deps->readSavedSettings)) {
        deps->errorHandler();
    }
    if (!usbInit(
            &usbManager,
            &modeManager,
            &settingsManager,
            deps->enterDFU,
            deps->saveSettings,
            deps->saveMode,
            deps->usbCdcReadTask,
            deps->usbWriteToSerial)) {
        deps->errorHandler();
    }

    if (!bq25180Init(
            &chargerIC,
            internalI2cReadRegisters,
            internalI2cWriteRegister,
            (0x6A << 1),
            internalLog,
            &caseLed,
            deps->enableTimers)) {
        deps->errorHandler();
    }

    configureChipState(
        &modeManager,
        &settingsManager.currentSettings,
        &button,
        &chargerIC,
        &accel,
        &caseLed,
        internalLog,
        deps->convertTicksToMilliseconds);
}

void microLightTask(void) {
    usbTask(&usbManager);
    stateTask();
}
