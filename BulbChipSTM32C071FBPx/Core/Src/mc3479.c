#include "mc3479.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>


// TODO: take derivative over a few different time frames and see if any are greater than threshold?
static const unsigned long kSampleIntervalTicks = 2UL; // TODO: dial this in after 6 byte fast read, see data sheet
static const float kSensitivityLsbPerG = 2048.0f;

/* Small helper to safely call the optional USB logging callback */
static void mc3479Log(MC3479 *dev, const char *msg) {
    if (dev == NULL || dev->writeToUsbSerial == NULL || msg == NULL) {
        return;
    }

    dev->writeToUsbSerial(0, msg, strlen(msg));
}

void mc3479Init(MC3479 *dev, MC3479ReadRegister *readCb, MC3479WriteRegister *writeCb, uint8_t devAddress) {
    if (!dev || !readCb || !writeCb) return;

    dev->readRegister = readCb;
    dev->writeRegister = writeCb;
    dev->devAddress = devAddress;
    dev->writeToUsbSerial = NULL;

    dev->currentMagnitudeG = 0.0f;
    dev->currentJerkGPerTick = 0.0f;
    dev->enabled = false;
    dev->lastSampleTick = 0;

    dev->lastAxG = 0.0f;
    dev->lastAyG = 0.0f;
    dev->lastAzG = 0.0f;

    // make sure in STANDBY when configuring
    dev->writeRegister(dev, MC3479_REG_CTRL1, 0x00);
    // set +/- 16g
    dev->writeRegister(dev, MC3479_REG_RANGE, 0b00110000);
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

static int16_t mc3479ReadAxis(MC3479 *dev, uint8_t reg_l, uint8_t reg_h) {
    if (!dev || !dev->readRegister) return 0;

    int8_t l = dev->readRegister(dev, reg_l);
    int8_t h = dev->readRegister(dev, reg_h);

    // Assemble as signed 16-bit two's complement
    int16_t raw = (int16_t)(((uint8_t)h << 8) | (uint8_t)l);
    return raw;
}

bool mc3479SampleNow(MC3479 *dev, unsigned long nowTicks) {
    if (!dev || !dev->readRegister || !dev->enabled) return false;

    // TODO: convert to 6 byte read, check data sheet
    int16_t raw_x = mc3479ReadAxis(dev, MC3479_REG_XOUT_L, MC3479_REG_XOUT_H);
    int16_t raw_y = mc3479ReadAxis(dev, MC3479_REG_YOUT_L, MC3479_REG_YOUT_H);
    int16_t raw_z = mc3479ReadAxis(dev, MC3479_REG_ZOUT_L, MC3479_REG_ZOUT_H);

    // Convert to g using configured sensitivity
    float gx = ((float)raw_x) / kSensitivityLsbPerG;
    float gy = ((float)raw_y) / kSensitivityLsbPerG;
    float gz = ((float)raw_z) / kSensitivityLsbPerG;

    // Compute magnitude
    dev->currentMagnitudeG = sqrtf(gx*gx + gy*gy + gz*gz);

    // Compute jerk (derivative of acceleration). This driver measures and
    // stores jerk in units of g per tick (caller ticks are used directly).
    if (dev->lastSampleTick != 0 && nowTicks > dev->lastSampleTick) {
        float dt_ticks = (float)(nowTicks - dev->lastSampleTick);
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

    dev->lastSampleTick = nowTicks;

    return true;
}

void mc3479Task(MC3479 *dev, unsigned long nowTicks) {
    if (!dev) return;
    if (!dev->enabled) return;

    if ((nowTicks - dev->lastSampleTick) >= kSampleIntervalTicks) {
        // Try to sample; if it fails, we leave the previous value intact
        if (mc3479SampleNow(dev, nowTicks)) {
            // sample_now updates last_sample_tick
        } else {
            mc3479Log(dev, "mc3479: sample failed\n");
            // Advance the last_sample_tick anyway to avoid continuous retries
            dev->lastSampleTick = nowTicks;
        }
    }
}
