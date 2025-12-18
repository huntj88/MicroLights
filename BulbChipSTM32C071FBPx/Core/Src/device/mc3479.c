#include "device/mc3479.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// TODO: set decimation rate
static const float kSensitivityLsbPerG = 2048.0f;

/* Small helper to safely call the optional USB logging callback */
static void mc3479Log(MC3479 *dev, const char *msg) {
    if (dev == NULL || dev->writeToUsbSerial == NULL || msg == NULL) {
        return;
    }

    dev->writeToUsbSerial(0, msg, strlen(msg));
}

bool mc3479Init(MC3479 *dev, MC3479ReadRegisters *readRegsCb, MC3479WriteRegister *writeCb, uint8_t devAddress, WriteToUsbSerial *writeToUsbSerial) {
    if (!dev || !readRegsCb || !writeCb || !writeToUsbSerial) return false;

    dev->readRegisters = readRegsCb;
    dev->writeRegister = writeCb;
    dev->devAddress = devAddress;
    dev->writeToUsbSerial = writeToUsbSerial;

    // make sure in STANDBY when configuring
    mc3479Disable(dev);

    // set +/- 16g
    dev->writeRegister(dev, MC3479_REG_RANGE, 0b00110000);
    return true;
}

void mc3479Enable(MC3479 *dev) {
    if (!dev || !dev->writeRegister) return;

    // put into WAKE mode
    dev->writeRegister(dev, MC3479_REG_CTRL1, 0b00000001);
    dev->enabled = true;

    // reset last sample tick so the task may sample immediately on next mc3479Task call
    dev->lastSampleTick = 0;
    dev->currentJerkGPerTick = 0.0f;
    dev->lastAxG = 0.0f;
    dev->lastAyG = 0.0f;
    dev->lastAzG = 0.0f;
}

void mc3479Disable(MC3479 *dev) {
    if (!dev || !dev->writeRegister) return;

    // Put the sensor into low-power / standby if supported
    dev->writeRegister(dev, MC3479_REG_CTRL1, 0x00);
    dev->enabled = false;

    // reset the sample time and clear the cached magnitude
    dev->lastSampleTick = 0;
    dev->currentMagnitudeG = 0.0f;
    dev->currentJerkGPerTick = 0.0f;
    dev->lastAxG = 0.0f;
    dev->lastAyG = 0.0f;
    dev->lastAzG = 0.0f;
}

bool mc3479SampleNow(MC3479 *dev, uint16_t tick) {
    if (!dev || !dev->enabled) return false;

    // read all 6 bytes
    uint8_t buf[6] = {0};
    if (dev->readRegisters) {
        // readRegisters should read 6 bytes starting at MC3479_REG_XOUT_L
    	bool read_ok = dev->readRegisters(dev, MC3479_REG_XOUT_L, buf, 6);
    	if (!read_ok) return false;
    }

    // Assemble as signed 16-bit two's complement
    int16_t raw_x = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]);
    int16_t raw_y = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]);
    int16_t raw_z = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]);

    // Convert to g using configured sensitivity
    float gx = ((float)raw_x) / kSensitivityLsbPerG;
    float gy = ((float)raw_y) / kSensitivityLsbPerG;
    float gz = ((float)raw_z) / kSensitivityLsbPerG;

    // Compute magnitude
    dev->currentMagnitudeG = sqrtf(gx*gx + gy*gy + gz*gz);

    // Compute jerk (derivative of acceleration). This driver measures and
    // stores jerk in units of g per tick (caller ticks are used directly).
    if (dev->lastSampleTick != 0 && tick > dev->lastSampleTick) {
        float dt_ticks = (float)(tick - dev->lastSampleTick);
        if (dt_ticks > 0.0f) {
            float dax = gx - dev->lastAxG;
            float day = gy - dev->lastAyG;
            float daz = gz - dev->lastAzG;

            float jerk_mag = sqrtf(dax*dax + day*day + daz*daz) / dt_ticks;
            dev->currentJerkGPerTick = jerk_mag;
        } else {
            dev->currentJerkGPerTick = 0.0f;
        }
    } else {
        // Insufficient history to compute jerk yet
        dev->currentJerkGPerTick = 0.0f;
    }

    dev->lastAxG = gx;
    dev->lastAyG = gy;
    dev->lastAzG = gz;

    dev->lastSampleTick = tick;

    return true;
}

void mc3479Task(MC3479 *dev, uint16_t tick, float millisPerTick) {
    if (!dev) return;
    if (!dev->enabled) return;

    // If at least 50 milliseconds have elapsed since the last sample, take a new one
    bool samplePeriodElapsed = (tick - dev->lastSampleTick) * millisPerTick >= 50;
    if (samplePeriodElapsed) {
        // Try to sample; if it fails, we leave the previous value intact
        if (mc3479SampleNow(dev, tick)) {
            // sample_now updates last_sample_tick
        } else {
            mc3479Log(dev, "mc3479: sample failed\n");
            // Advance the last_sample_tick anyway to avoid continuous retries
            dev->lastSampleTick = tick;
        }
    }
}

bool isOverThreshold(MC3479 *dev, float threshold) {
	if (!dev) return false;
	if (!dev->enabled) return false;

	return dev->currentJerkGPerTick > threshold;
}
