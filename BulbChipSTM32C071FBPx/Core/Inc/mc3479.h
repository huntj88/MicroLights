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

// Axis output registers
#define MC3479_REG_XOUT_L    0x0D
#define MC3479_REG_XOUT_H    0x0E
#define MC3479_REG_YOUT_L    0x0F
#define MC3479_REG_YOUT_H    0x10
#define MC3479_REG_ZOUT_L    0x11
#define MC3479_REG_ZOUT_H    0x12

typedef struct MC3479 MC3479; // forward declaration

// Read multiple consecutive registers. Returns true on success.
typedef bool MC3479ReadRegisters(MC3479 *dev, uint8_t startReg, uint8_t *buf, size_t len);

// Write a single 8-bit register.
typedef void MC3479WriteRegister(MC3479 *dev, uint8_t reg, uint8_t value);

// TODO: consolidate this interface with the duplicates
typedef void WriteToUsbSerial(uint8_t itf, const char *buf, uint32_t count);

struct MC3479 {
    MC3479ReadRegisters *readRegisters;
    MC3479WriteRegister *writeRegister;
    WriteToUsbSerial *writeToUsbSerial;

    uint8_t devAddress;

    // Current calculated magnitude in units of g
    float currentMagnitudeG; // TODO: delete?

    // Current calculated jerk magnitude (absolute) in units of g per tick
    // Caller-provided ticks are used directly (no conversion to seconds).
    float currentJerkGPerTick;

    bool enabled;
    uint16_t lastSampleTick;

    // Cached last acceleration values in units of g (for jerk calculation)
    float lastAxG;
    float lastAyG;
    float lastAzG;
};


bool mc3479Init(MC3479 *dev, MC3479ReadRegisters *readRegsCb, MC3479WriteRegister *writeCb, uint8_t devAddress, WriteToUsbSerial *writeToUsbSerial);

void mc3479Enable(MC3479 *dev);
void mc3479Disable(MC3479 *dev);

// Polling task: call periodically from the main loop. When enabled and the
// loop interval has elapsed this will read the three axis values, compute
// magnitude and update `currentMagnitudeG`.
void mc3479Task(MC3479 *dev, uint16_t tick, float millisPerTick);

// Force an immediate sample and magnitude calculation. Returns true on success,
// false if a sample couldn't be taken (e.g. missing read callback).
// The caller must provide the current tick value (same units as used by
// mc3479Task). Jerk is computed as change-in-acceleration divided by
// delta-ticks, and therefore its units are g per tick (g/tick).
bool mc3479SampleNow(MC3479 *dev, uint16_t tick);

bool isOverThreshold(MC3479 *dev, float threshold);

#endif /* INC_MC3479_H_ */
