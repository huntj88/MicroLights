#include "microlight/microlight.h"
#include <stdio.h>
#include <string.h>
#include "microlight/i2c_log_decorate.h"
#include "microlight/json/json_buf.h"

static volatile bool buttonInterruptTriggered = false;
static volatile bool chargerInterruptTriggered = false;
static volatile bool autoOffTimerInterruptTriggered = false;
static volatile uint32_t microLightTicks = 0;

static uint32_t (*convertTicksToMilliseconds)(uint32_t ticks) = NULL;

static BQ25180 chargerIC;
static Button button;
static MC3479 accel;
static RGBLed caseLed;
static ModeManager modeManager;
static SettingsManager settingsManager;
static USBManager usbManager;
static ChipState chipState;

static void internalLog(const char *buffer, size_t length) {
    if (usbManager.usbWriteToSerial != NULL) {
        usbManager.usbWriteToSerial(buffer, length);
    }
}

// Wrap I2C dependencies to handle logging internal to this file
static I2CWriteRegisterChecked rawI2cWrite = NULL;
static I2CReadRegisters rawI2cReadRegs = NULL;

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
    uint8_t devAddress, uint8_t startReg, uint8_t *buffer, size_t length) {
    return i2cDecoratedReadRegisters(
        devAddress,
        startReg,
        buffer,
        length,
        rawI2cReadRegs,
        &settingsManager.currentSettings.enableI2cFailureReporting,
        internalLog);
}

// TODO: return bool and handle error externally, remove errorHandler from deps
void configureMicroLight(MicroLightDependencies *deps) {
    if (!deps->convertTicksToMilliseconds || !deps->i2cReadRegisters) {
        deps->errorHandler();
    }
    convertTicksToMilliseconds = deps->convertTicksToMilliseconds;

    if (!deps->i2cWriteRegister || !deps->i2cReadRegisters) {
        deps->errorHandler();
    }
    rawI2cWrite = deps->i2cWriteRegister;
    rawI2cReadRegs = deps->i2cReadRegisters;

    if (!initSharedJsonIOBuffer(deps->jsonBuffer, deps->jsonBufferSize)) {
        deps->errorHandler();
    }

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

    if (!configureChipState(
            &chipState,
            &modeManager,
            &settingsManager.currentSettings,
            &button,
            &chargerIC,
            &accel,
            &caseLed,
            internalLog)) {
        deps->errorHandler();
    }
}

void microLightTask(void) {
    usbTask(&usbManager);

    uint32_t milliseconds = convertTicksToMilliseconds(microLightTicks);

    // Potentially could miss an interrupt if it occurs after local copy of false, but set to true
    // in interrupt before clearing. Would need to add critical section mcu dependency to prevent,
    // but this is low probability.
    bool autoOffITLocal = autoOffTimerInterruptTriggered;
    autoOffTimerInterruptTriggered = false;
    bool buttonITLocal = buttonInterruptTriggered;
    buttonInterruptTriggered = false;
    bool chargerITLocal = chargerInterruptTriggered;
    chargerInterruptTriggered = false;

    stateTask(
        &chipState,
        milliseconds,
        (StateTaskFlags){
            .autoOffTimerInterruptTriggered = autoOffITLocal,
            .buttonInterruptTriggered = buttonITLocal,
            .chargerInterruptTriggered = chargerITLocal});
}

void microLightInterrupt(enum MicroLightInterrupt interrupt) {
    switch (interrupt) {
        case ButtonInterrupt:
            buttonInterruptTriggered = true;
            break;
        case ChargerInterrupt:
            chargerInterruptTriggered = true;
            break;
        case ChipTickInterrupt:
            microLightTicks++;
            break;
        case AutoOffTimerInterrupt:
            autoOffTimerInterruptTriggered = true;
            break;
    }
}
