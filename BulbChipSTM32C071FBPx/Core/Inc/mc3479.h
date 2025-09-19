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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MC3479_I2CADDR_DEFAULT 0x99 // 8-bit address

// Register map used by this driver (defaults - consult datasheet)
#define MC3479_REG_STATUS    0x05
#define MC3479_REG_CTRL1     0x07
#define MC3479_REG_CTRL2     0x08
#define MC3479_REG_RANGE     0x20

// Axis output registers (LSB/ MSB ordering inferred from project use)
#define MC3479_REG_XOUT_L    0x0D
#define MC3479_REG_XOUT_H    0x0E
#define MC3479_REG_YOUT_L    0x0F
#define MC3479_REG_YOUT_H    0x10
#define MC3479_REG_ZOUT_L    0x11
#define MC3479_REG_ZOUT_H    0x12

typedef struct MC3479 MC3479; // forward declaration

// Read one 8-bit register. Implementations should return the raw byte as
// a signed 8-bit value (two's complement behaviour preserved when assembling
// 16-bit axis values).
typedef int8_t MC3479ReadRegister(MC3479 *dev, uint8_t reg);
// Write a single 8-bit register.
typedef void MC3479WriteRegister(MC3479 *dev, uint8_t reg, uint8_t value);
// Optional short logging callback (mirrors pattern in bq25180).
typedef void MC3479WriteToUsbSerial(uint8_t itf, const char *buf, unsigned long count);

struct MC3479 {
    MC3479ReadRegister *readRegister;
    MC3479WriteRegister *writeRegister;
    MC3479WriteToUsbSerial *writeToUsbSerial; // optional, may be NULL

    uint8_t devAddress;

    // Current calculated magnitude in units of g
    float currentMagnitudeG;

    // Current calculated jerk magnitude (absolute) in units of g per tick
    // Caller-provided ticks are used directly (no conversion to seconds).
    float currentJerkGPerTick;

    bool enabled;
    unsigned long lastSampleTick; // tick value of last sample

    // Cached last acceleration values in units of g (for jerk calculation)
    float lastAxG;
    float lastAyG;
    float lastAzG;
};


void mc3479Init(MC3479 *dev, MC3479ReadRegister *readCb, MC3479WriteRegister *writeCb, uint8_t devAddress);

void mc3479Enable(MC3479 *dev);
void mc3479Disable(MC3479 *dev);

// Polling task: call periodically from the main loop. When enabled and the
// loop interval has elapsed this will read the three axis values, compute
// magnitude and update `currentMagnitudeG`.
void mc3479Task(MC3479 *dev, unsigned long nowTicks);

// Get most-recent magnitude in g. This is a snapshot of the last
// measurement performed by mc3479Task() or mc3479SampleNow().
float mc3479GetMagnitude(MC3479 *dev);

// Force an immediate sample and magnitude calculation. Returns 0 on success,
// non-zero if a sample couldn't be taken (e.g. missing read callback).
// The caller must provide the current tick value (same units as used by
// mc3479Task). Jerk is computed as change-in-acceleration divided by
// delta-ticks, and therefore its units are g per tick (g/tick).
int mc3479SampleNow(MC3479 *dev, unsigned long nowTicks);

// Return the absolute magnitude of the most recently computed jerk value
// in units of g per tick. Returns 0 if insufficient samples exist.
float mc3479GetJerkMagnitude(MC3479 *dev);

#endif /* INC_MC3479_H_ */
