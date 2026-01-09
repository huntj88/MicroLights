#include "microlight/device/mc3479.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// TODO: set decimation rate?
// Sensitivity for +/- 16g range: 32768 / 16 = 2048 LSB/g
#define MC3479_SENSITIVITY_LSB_PER_G 2048ULL

/* Small helper to safely call the optional USB logging callback */
static void mc3479Log(MC3479 *dev, const char *msg) {
    if (dev == NULL || dev->writeToSerial == NULL || msg == NULL) {
        return;
    }

    dev->writeToSerial(msg, strlen(msg));
}

bool mc3479Init(
    MC3479 *dev,
    I2CReadRegisters *readRegsCb,
    I2CWriteRegister *writeCb,
    uint8_t devAddress,
    WriteToSerial *writeToSerial) {
    if (!dev || !readRegsCb || !writeCb || !writeToSerial) {
        return false;
    }

    dev->readRegisters = readRegsCb;
    dev->writeRegister = writeCb;
    dev->devAddress = devAddress;
    dev->writeToSerial = writeToSerial;

    // make sure in STANDBY when configuring
    mc3479Disable(dev);

    // set +/- 16g
    dev->writeRegister(dev->devAddress, MC3479_REG_RANGE, 0b00110000);
    return true;
}

void mc3479Enable(MC3479 *dev) {
    if (!dev || !dev->writeRegister) {
        return;
    }

    // put into WAKE mode
    dev->writeRegister(dev->devAddress, MC3479_REG_CTRL1, 0b00000001);
    dev->enabled = true;

    // reset last sample tick so the task may sample immediately on next mc3479Task call
    dev->lastSampleMs = 0;
    dev->currentJerkSquaredSum = 0;
    dev->lastDtMs = 0;
    dev->lastRawX = 0;
    dev->lastRawY = 0;
    dev->lastRawZ = 0;
}

void mc3479Disable(MC3479 *dev) {
    if (!dev || !dev->writeRegister) {
        return;
    }

    // Put the sensor into low-power / standby if supported
    dev->writeRegister(dev->devAddress, MC3479_REG_CTRL1, 0x00);
    dev->enabled = false;

    // reset the sample time and clear the cached magnitude
    dev->lastSampleMs = 0;
    dev->currentJerkSquaredSum = 0;
    dev->lastDtMs = 0;
    dev->lastRawX = 0;
    dev->lastRawY = 0;
    dev->lastRawZ = 0;
}

bool mc3479SampleNow(MC3479 *dev, uint32_t milliseconds) {
    if (!dev || !dev->enabled) {
        return false;
    }

    // read all 6 bytes
    uint8_t buf[6] = {0};
    if (dev->readRegisters) {
        // readRegisters should read 6 bytes starting at MC3479_REG_XOUT_L
        bool read_ok = dev->readRegisters(dev->devAddress, MC3479_REG_XOUT_L, buf, 6);
        if (!read_ok) {
            return false;
        }
    }

    // Assemble as signed 16-bit two's complement
    int16_t raw_x = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]);
    int16_t raw_y = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]);
    int16_t raw_z = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]);

    // Compute jerk (derivative of acceleration). This driver measures and
    // stores jerk in units of g per ms (caller ms are used directly).
    if (dev->lastSampleMs != 0 && milliseconds > dev->lastSampleMs) {
        uint32_t dt_ms = milliseconds - dev->lastSampleMs;
        if (dt_ms > 0) {
            int32_t dax = (int32_t)raw_x - dev->lastRawX;
            int32_t day = (int32_t)raw_y - dev->lastRawY;
            int32_t daz = (int32_t)raw_z - dev->lastRawZ;

            dev->currentJerkSquaredSum =
                (uint64_t)dax * dax + (uint64_t)day * day + (uint64_t)daz * daz;
            dev->lastDtMs = dt_ms;
        } else {
            dev->currentJerkSquaredSum = 0;
            dev->lastDtMs = 0;
        }
    } else {
        // Insufficient history to compute jerk yet
        dev->currentJerkSquaredSum = 0;
        dev->lastDtMs = 0;
    }

    dev->lastRawX = raw_x;
    dev->lastRawY = raw_y;
    dev->lastRawZ = raw_z;

    dev->lastSampleMs = milliseconds;

    return true;
}

void mc3479Task(MC3479 *dev, uint32_t milliseconds) {
    if (!dev || !dev->enabled) {
        return;
    }

    // If at least 50 milliseconds have elapsed since the last sample, take a new one
    uint32_t elapsed = milliseconds - dev->lastSampleMs;
    bool samplePeriodElapsed = elapsed >= 50;
    if (samplePeriodElapsed) {
        // Try to sample; if it fails, we leave the previous value intact
        if (mc3479SampleNow(dev, milliseconds)) {
            // sample_now updates last_sample_tick
        } else {
            mc3479Log(dev, "mc3479: sample failed\n");
            // Advance the last_sample_tick anyway to avoid continuous retries
            dev->lastSampleMs = milliseconds;
        }
    }
}

bool isOverThreshold(MC3479 *dev, uint8_t threshold) {
    if (!dev || !dev->enabled || dev->lastDtMs == 0) {
        return false;
    }

    // Threshold is in G/s.
    // We want to check if:
    // sqrt(sum_sq_diff) / 2048 / dt_ms * 1000 > threshold
    //
    // Squaring both sides:
    // sum_sq_diff * 1000000 > (threshold * 2048 * dt_ms)^2

    uint64_t lhs = dev->currentJerkSquaredSum * 1000000ULL;
    uint64_t rhs_term =
        (uint64_t)threshold * MC3479_SENSITIVITY_LSB_PER_G * (uint64_t)dev->lastDtMs;
    uint64_t rhs = rhs_term * rhs_term;

    return lhs > rhs;
}
