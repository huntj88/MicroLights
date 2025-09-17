#include <stdint.h>
#include "mc3479.h"
#include <math.h>

// Small helper to safely call the optional USB logging callback
static void mc3479_log(MC3479 *dev, const char *msg) {
    if (dev && dev->writeToUsbSerial && msg) {
        unsigned long len = 0;
        while (msg[len]) len++;
        dev->writeToUsbSerial(0, msg, len);
    }
}

void mc3479_init(MC3479 *dev, Mc3479ReadRegister *readCb, Mc3479WriteRegister *writeCb, uint8_t devAddress) {
    if (!dev || !readCb || !writeCb) return;

    dev->readRegister = readCb;
    dev->writeRegister = writeCb;
    dev->devAddress = devAddress;
    dev->writeToUsbSerial = NULL;

    dev->current_magnitude_g = 0.0f;
    dev->enabled = 0;
    dev->last_sample_tick = 0;

    dev->last_x = 0;
    dev->last_y = 0;
    dev->last_z = 0;

    // make sure in STANDYBY when configuring
    dev->writeRegister(dev, MC3479_REG_CTRL1, 0x00);
    // set +/- 16g
    dev->writeRegister(dev, MC3479_REG_RANGE, 0b00110000);
}

void mc3479_enable(MC3479 *dev) {
	if (!dev || !dev->writeRegister) return;

	// put into WAKE mode
    dev->writeRegister(dev, MC3479_REG_CTRL1, 0b00000001);
    dev->enabled = 1;
    // reset last sample tick so the task may sample immediately on next mc3479_task call
    dev->last_sample_tick = 0;
}

void mc3479_disable(MC3479 *dev) {
	if (!dev || !dev->readRegister) return;

    // Put the sensor into low-power / standby if supported
    dev->writeRegister(dev, MC3479_REG_CTRL1, 0x00);
    dev->enabled = 0;

    // reset the sample time and clear the cached magnitude
    dev->last_sample_tick = 0;
    dev->current_magnitude_g = 0;
}

static int16_t mc3479_read_axis(MC3479 *dev, uint8_t reg_l, uint8_t reg_h) {
    if (!dev || !dev->readRegister) return 0;

    int8_t l = dev->readRegister(dev, reg_l);
    int8_t h = dev->readRegister(dev, reg_h);

    // Assemble as signed 16-bit two's complement
    int16_t raw = (int16_t)(((uint8_t)h << 8) | (uint8_t)l);
    return raw;
}

int mc3479_sample_now(MC3479 *dev) {
    if (!dev || !dev->readRegister || !dev->enabled) return -1;

    int16_t raw_x = mc3479_read_axis(dev, MC3479_REG_XOUT_L, MC3479_REG_XOUT_H);
    int16_t raw_y = mc3479_read_axis(dev, MC3479_REG_YOUT_L, MC3479_REG_YOUT_H);
    int16_t raw_z = mc3479_read_axis(dev, MC3479_REG_ZOUT_L, MC3479_REG_ZOUT_H);

    dev->last_x = raw_x;
    dev->last_y = raw_y;
    dev->last_z = raw_z;

    // Convert to g using configured sensitivity
    float gx = ((float)raw_x) / 2048;
    float gy = ((float)raw_y) / 2048;
    float gz = ((float)raw_z) / 2048;

    dev->current_magnitude_g = sqrtf(gx*gx + gy*gy + gz*gz);

    return 0;
}

void mc3479_task(MC3479 *dev, unsigned long now_ticks) {
    if (!dev) return;
    if (!dev->enabled) return;

    if ((now_ticks - dev->last_sample_tick) >= 100) {
        // Try to sample; if it fails, we leave the previous value intact
        if (mc3479_sample_now(dev) == 0) {
            dev->last_sample_tick = now_ticks;
        } else {
            mc3479_log(dev, "mc3479: sample failed\n");
            dev->last_sample_tick = now_ticks; // advance anyway to avoid continuous retries
        }
    }
}

float mc3479_get_magnitude(MC3479 *dev) {
    if (!dev) return 0.0f;
    return dev->current_magnitude_g;
}
