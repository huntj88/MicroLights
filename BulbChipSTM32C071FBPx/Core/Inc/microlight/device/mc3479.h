/*
 * mc3479.h
 *
 * Abstraction for the MC3479 I2C accelerometer.
 *
 * Provides a simple interface to enable/disable periodic sampling
 * and to read the most recent absolute acceleration magnitude in units of g.
 */

#ifndef INC_MC3479_H_
#define INC_MC3479_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "microlight/device/i2c.h"
#include "microlight/model/log.h"

#define MC3479_I2CADDR_DEFAULT 0x99  // 8-bit address

// Register map used by this driver (defaults - consult datasheet)
#define MC3479_REG_STATUS 0x05
#define MC3479_REG_CTRL1 0x07
#define MC3479_REG_CTRL2 0x08
#define MC3479_REG_RANGE 0x20

// Axis output registers
#define MC3479_REG_XOUT_L 0x0D
#define MC3479_REG_XOUT_H 0x0E
#define MC3479_REG_YOUT_L 0x0F
#define MC3479_REG_YOUT_H 0x10
#define MC3479_REG_ZOUT_L 0x11
#define MC3479_REG_ZOUT_H 0x12

typedef struct MC3479 MC3479;  // forward declaration

struct MC3479 {
    I2CReadRegisters readRegisters;
    I2CWriteRegister writeRegister;

    uint8_t devAddress;

    // Current calculated jerk magnitude (absolute) squared
    uint64_t currentJerkSquaredSum;
    uint32_t lastDtMs;

    bool enabled;
    uint32_t lastSampleMs;

    // Cached last acceleration values (raw)
    int16_t lastRawX;
    int16_t lastRawY;
    int16_t lastRawZ;
};

bool mc3479Init(
    MC3479 *dev,
    I2CReadRegisters readRegisters,
    I2CWriteRegister writeRegister,
    uint8_t devAddress);

void mc3479Enable(MC3479 *dev);
void mc3479Disable(MC3479 *dev);

// Polling task: call periodically from the main loop. When enabled and the
// loop interval has elapsed this will read the three axis values, compute
// magnitude and update `currentMagnitudeG`.
void mc3479Task(MC3479 *dev, uint32_t milliseconds);

// Force an immediate sample and magnitude calculation. Returns true on success,
// false if a sample couldn't be taken (e.g. missing read callback).
// The caller must provide the current ms value (same units as used by
// mc3479Task). Jerk is computed as change-in-acceleration divided by
// delta-ms, and therefore its units are g per ms (g/ms).
bool mc3479SampleNow(MC3479 *dev, uint32_t milliseconds);

bool isOverThreshold(MC3479 *dev, uint8_t threshold);

#endif /* INC_MC3479_H_ */
