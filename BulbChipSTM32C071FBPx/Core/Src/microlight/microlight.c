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
static RGBLed frontLed;
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

bool configureMicroLight(MicroLightDependencies *deps) {
    if (!deps || !deps->convertTicksToMilliseconds || !deps->i2cReadRegisters ||
        !deps->i2cWriteRegister || !deps->writeRgbPwmCaseLed || !deps->writeRgbPwmFrontLed ||
        !deps->readButtonPin || !deps->enableChipTickTimer || !deps->enableCaseLedTimer ||
        !deps->enableFrontLedTimer || !deps->startAutoOffTimer || !deps->readSavedMode ||
        !deps->writeBulbLed || !deps->readSavedSettings || !deps->enterDFU || !deps->saveSettings ||
        !deps->saveMode || !deps->usbCdcReadTask || !deps->usbWriteToSerial || !deps->jsonBuffer ||
        deps->jsonBufferSize == 0) {
        return false;
    }

    convertTicksToMilliseconds = deps->convertTicksToMilliseconds;
    rawI2cWrite = deps->i2cWriteRegister;
    rawI2cReadRegs = deps->i2cReadRegisters;

    if (!initSharedJsonIOBuffer(deps->jsonBuffer, deps->jsonBufferSize)) {
        return false;
    }

    if (!rgbInit(&caseLed, deps->writeRgbPwmCaseLed, (uint16_t)deps->rgbTimerPeriod)) {
        return false;
    }

    if (!rgbInit(&frontLed, deps->writeRgbPwmFrontLed, (uint16_t)deps->rgbTimerPeriod)) {
        return false;
    }

    if (!buttonInit(&button, deps->readButtonPin, deps->enableChipTickTimer, &caseLed)) {
        return false;
    }

    if (!mc3479Init(
            &accel, internalI2cReadRegisters, internalI2cWriteRegister, MC3479_I2CADDR_DEFAULT)) {
        return false;
    }

    if (!bq25180Init(
            &chargerIC,
            internalI2cReadRegisters,
            internalI2cWriteRegister,
            (0x6A << 1),
            internalLog,
            &caseLed)) {
        return false;
    }

    if (!modeManagerInit(
            &modeManager,
            &accel,
            &caseLed,
            &frontLed,
            deps->readSavedMode,
            deps->writeBulbLed,
            internalLog)) {
        return false;
    }

    if (!settingsManagerInit(&settingsManager, deps->readSavedSettings)) {
        return false;
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
        return false;
    }

    if (!configureChipState(
            &chipState,
            (ChipDependencies){
                .modeManager = &modeManager,
                .settings = &settingsManager.currentSettings,
                .button = &button,
                .caseLed = &caseLed,
                .chargerIC = &chargerIC,
                .accel = &accel,
                .enableChipTickTimer = deps->enableChipTickTimer,
                .enableCaseLedTimer = deps->enableCaseLedTimer,
                .enableFrontLedTimer = deps->enableFrontLedTimer,
                .log = internalLog,
            })) {
        return false;
    }

    deps->startAutoOffTimer();
    return true;
}

void microLightTask(void) {
    usbTask(&usbManager);

    // if convertTicksToMilliseconds is null, let it crash if not microlight is not configured
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
