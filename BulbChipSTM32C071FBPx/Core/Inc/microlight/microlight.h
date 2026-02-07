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
#include "microlight/model/usb.h"
#include "microlight/settings_manager.h"
#include "microlight/usb_manager.h"

typedef struct {
    // Hardware I/O
    I2CWriteRegisterChecked i2cWriteRegister;
    I2CReadRegisters i2cReadRegisters;
    RGBWritePwm writeRgbPwmCaseLed;
    RGBWritePwm writeRgbPwmFrontLed;
    void (*writeBulbLed)(uint8_t state);
    uint8_t (*readButtonPin)(void);

    // USB
    UsbCdcReadTask usbCdcReadTask;
    UsbWriteToSerial usbWriteToSerial;

    // Storage
    ReadSavedSettings readSavedSettings;
    SaveSettings saveSettings;
    ReadSavedMode readSavedMode;
    SaveMode saveMode;

    // System
    void (*enableChipTickTimer)(bool enable);
    void (*enableCaseLedTimer)(bool enable);
    void (*enableFrontLedTimer)(bool enable);
    void (*enterDFU)(void);
    uint32_t (*convertTicksToMilliseconds)(uint32_t ticks);
    uint32_t rgbTimerPeriod;

    // Memory
    char *jsonBuffer;
    size_t jsonBufferSize;
} MicroLightDependencies;

enum MicroLightInterrupt {
    ButtonInterrupt,
    ChargerInterrupt,
    ChipTickInterrupt,
    AutoOffTimerInterrupt
};

bool configureMicroLight(MicroLightDependencies *deps);
void microLightTask(void);
void microLightInterrupt(enum MicroLightInterrupt interrupt);

// TODO: integration test with no application mocks, just hardware mocks

#endif  // MICROLIGHT_H
