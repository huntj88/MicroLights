#ifndef MICROLIGHT_H
#define MICROLIGHT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "microlight/chip_state.h"
#include "microlight/device/bq25180.h"
#include "microlight/device/button.h"
#include "microlight/device/mc3479.h"
#include "microlight/device/rgb_led.h"
#include "microlight/mode_manager.h"
#include "microlight/settings_manager.h"
#include "microlight/usb_manager.h"

// TODO: reorder
typedef struct {
    void (*writeRgbPwmCaseLed)(uint16_t redDuty, uint16_t greenDuty, uint16_t blueDuty);
    uint8_t (*readButtonPin)(void);
    void (*enableTimers)(bool enable);
    uint8_t (*readRegister)(uint8_t devAddress, uint8_t reg);
    void (*writeRegister)(uint8_t devAddress, uint8_t reg, uint8_t value);
    bool (*readRegisters)(uint8_t devAddress, uint8_t startReg, uint8_t *buf, size_t len);
    void (*readModeFromFlash)(uint8_t mode, char buffer[], uint32_t length);
    void (*writeBulbLed)(uint8_t state);
    void (*readSettingsFromFlash)(char buffer[], uint32_t length);
    void (*enterDFU)(void);
    void (*writeSettingsToFlash)(const char str[], uint32_t length);
    void (*writeModeToFlash)(uint8_t mode, const char str[], uint32_t length);
    uint32_t (*convertTicksToMilliseconds)(uint32_t ticks);
    uint32_t timerPeriod; // For RGB LED
    void (*errorHandler)(void);
} MicroLightDependencies;

void configureMicroLight(MicroLightDependencies *deps);
void microLightTask(void);

#endif // MICROLIGHT_H
