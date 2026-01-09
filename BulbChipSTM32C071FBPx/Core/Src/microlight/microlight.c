#include "microlight/microlight.h"
#include <string.h>

static BQ25180 chargerIC;
static Button button;
static MC3479 accel;
static RGBLed caseLed;
static ModeManager modeManager;
static SettingsManager settingsManager;
static USBManager usbManager;

static void writeToSerial(const char *buf, uint32_t count) {
    usbWriteToSerial(&usbManager, 0, buf, count);
}

void configureMicroLight(MicroLightDependencies *deps) {
    if (!rgbInit(&caseLed, deps->writeRgbPwmCaseLed, (uint16_t)deps->timerPeriod)) {
        deps->errorHandler();
    }

    if (!buttonInit(&button, deps->readButtonPin, deps->enableTimers, &caseLed)) {
        deps->errorHandler();
    }

    if (!mc3479Init(&accel, deps->readRegisters, deps->writeRegister, MC3479_I2CADDR_DEFAULT, writeToSerial)) {
        deps->errorHandler();
    }

    if (!modeManagerInit(
            &modeManager,
            &accel,
            &caseLed,
            deps->enableTimers,
            deps->readModeFromFlash,
            deps->writeBulbLed,
            writeToSerial)) {
        deps->errorHandler();
    }
    if (!settingsManagerInit(&settingsManager, deps->readSettingsFromFlash)) {
        deps->errorHandler();
    }
    if (!usbInit(
            &usbManager,
            &modeManager,
            &settingsManager,
            deps->enterDFU,
            deps->writeSettingsToFlash,
            deps->writeModeToFlash)) {
        deps->errorHandler();
    }

    if (!bq25180Init(
            &chargerIC,
            deps->readRegister,
            deps->writeRegister,
            (0x6A << 1),
            writeToSerial,
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
        writeToSerial,
        deps->convertTicksToMilliseconds);
}

void microLightTask(void) {
    usbCdcTask(&usbManager);
    stateTask();
}
