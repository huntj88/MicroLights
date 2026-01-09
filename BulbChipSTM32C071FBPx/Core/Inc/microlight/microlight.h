#ifndef MICROLIGHT_H
#define MICROLIGHT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "microlight/chip_state.h"
#include "microlight/device/bq25180.h"
#include "microlight/device/button.h"
#include "microlight/device/i2c.h"
#include "microlight/device/mc3479.h"
#include "microlight/device/rgb_led.h"
#include "microlight/mode_manager.h"
#include "microlight/model/storage.h"
#include "microlight/settings_manager.h"
#include "microlight/usb_manager.h"

typedef struct {
    // Hardware I/O
    I2CWriteRegisterChecked *i2cWriteRegister;
    I2CReadRegister *i2cReadRegister;
    I2CReadRegisters *i2cReadRegisters;
    RGBWritePwm *writeRgbPwmCaseLed;
    void (*writeBulbLed)(uint8_t state);
    uint8_t (*readButtonPin)(void);

    // Storage
    ReadSavedSettings readSettingsFromFlash;
    SaveSettings writeSettingsToFlash;
    ReadSavedMode readModeFromFlash;
    SaveMode writeModeToFlash;

    // System
    void (*enableTimers)(bool enable);
    void (*enterDFU)(void);
    uint32_t (*convertTicksToMilliseconds)(uint32_t ticks);
    void (*errorHandler)(void);
    uint32_t rgbTimerPeriod;
} MicroLightDependencies;

void configureMicroLight(MicroLightDependencies *deps);
void microLightTask(void);

// TODO: integration test with no application mocks, just hardware mocks
// TODO: unit test i2c logging decorator

#endif  // MICROLIGHT_H
