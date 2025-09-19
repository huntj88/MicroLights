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

#define MC3479_I2CADDR_DEFAULT 0x99 // 8-bit address

// Register map used by this driver (defaults - consult datasheet)
#define MC3479_REG_STATUS    0x05
#define MC3479_REG_CTRL1     0x07
#define MC3479_REG_CTRL2     0x08
#define MC3479_REG_RANGE     0X20

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
typedef int8_t Mc3479ReadRegister(MC3479 *dev, uint8_t reg);
// Write a single 8-bit register.
typedef void Mc3479WriteRegister(MC3479 *dev, uint8_t reg, uint8_t value);
// Optional short logging callback (mirrors pattern in bq25180).
typedef void Mc3479WriteToUsbSerial(uint8_t itf, const char *buf, unsigned long count);

struct MC3479 {
    Mc3479ReadRegister *readRegister;
    Mc3479WriteRegister *writeRegister;
    Mc3479WriteToUsbSerial *writeToUsbSerial; // optional, may be NULL

    uint8_t devAddress;

    // Current calculated magnitude in units of g
    float current_magnitude_g;

    // Current calculated jerk magnitude (absolute) in units of g per tick
    // Caller-provided ticks are used directly (no conversion to seconds).
    float current_jerk_g_per_tick;

    uint8_t enabled;
    unsigned long last_sample_tick; // tick value of last sample

    // Cached last acceleration values in units of g (for jerk calculation)
    float last_ax_g;
    float last_ay_g;
    float last_az_g;
};


void mc3479_init(MC3479 *dev, Mc3479ReadRegister *readCb, Mc3479WriteRegister *writeCb, uint8_t devAddress);

void mc3479_enable(MC3479 *dev);
void mc3479_disable(MC3479 *dev);

// Polling task: call periodically from the main loop. When enabled and the
// loop interval has elapsed this will read the three axis values, compute
// magnitude and update `current_magnitude_g`.
void mc3479_task(MC3479 *dev, unsigned long now_ticks);

// Get most-recent magnitude (g). This is just a snapshot of the last
// measurement performed by mc3479_task() and may be 0 if no sample has
// been taken yet.
float mc3479_get_magnitude(MC3479 *dev);

// Force an immediate sample and magnitude calculation. Returns 0 on success,
// non-zero if a sample couldn't be taken (e.g. missing read callback).
// The caller must provide the current tick value (same units as used by
// mc3479_task). Jerk is computed as change-in-acceleration divided by
// delta-ticks, and therefore its units are g per tick (g/tick).
int mc3479_sample_now(MC3479 *dev, unsigned long now_ticks);

// Return the absolute magnitude of the most recently computed jerk value
// in units of g per tick. Returns 0 if insufficient samples exist.
float mc3479_get_jerk_magnitude(MC3479 *dev);

#endif /* INC_MC3479_H_ */
