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
    dev->current_jerk_g_per_tick = 0.0f;
    dev->enabled = 0;
    dev->last_sample_tick = 0;

    dev->last_ax_g = 0.0f;
    dev->last_ay_g = 0.0f;
    dev->last_az_g = 0.0f;

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
    dev->current_jerk_g_per_tick = 0.0f;
    dev->last_ax_g = 0.0f;
    dev->last_ay_g = 0.0f;
    dev->last_az_g = 0.0f;
}

void mc3479_disable(MC3479 *dev) {
	if (!dev || !dev->readRegister) return;

    // Put the sensor into low-power / standby if supported
    dev->writeRegister(dev, MC3479_REG_CTRL1, 0x00);
    dev->enabled = 0;

    // reset the sample time and clear the cached magnitude
    dev->last_sample_tick = 0;
    dev->current_magnitude_g = 0;
    dev->current_jerk_g_per_tick = 0.0f;
    dev->last_ax_g = 0.0f;
    dev->last_ay_g = 0.0f;
    dev->last_az_g = 0.0f;
}

static int16_t mc3479_read_axis(MC3479 *dev, uint8_t reg_l, uint8_t reg_h) {
    if (!dev || !dev->readRegister) return 0;

    int8_t l = dev->readRegister(dev, reg_l);
    int8_t h = dev->readRegister(dev, reg_h);

    // Assemble as signed 16-bit two's complement
    int16_t raw = (int16_t)(((uint8_t)h << 8) | (uint8_t)l);
    return raw;
}

int mc3479_sample_now(MC3479 *dev, unsigned long now_ticks) {
    if (!dev || !dev->readRegister || !dev->enabled) return -1;

    int16_t raw_x = mc3479_read_axis(dev, MC3479_REG_XOUT_L, MC3479_REG_XOUT_H);
    int16_t raw_y = mc3479_read_axis(dev, MC3479_REG_YOUT_L, MC3479_REG_YOUT_H);
    int16_t raw_z = mc3479_read_axis(dev, MC3479_REG_ZOUT_L, MC3479_REG_ZOUT_H);

    // Convert to g using configured sensitivity
    float gx = ((float)raw_x) / 2048.0f;
    float gy = ((float)raw_y) / 2048.0f;
    float gz = ((float)raw_z) / 2048.0f;

    // Compute magnitude
    dev->current_magnitude_g = sqrtf(gx*gx + gy*gy + gz*gz);

    // Compute jerk (derivative of acceleration). This driver measures and
    // stores jerk in units of g per tick (caller ticks are used directly).
    if (dev->last_sample_tick != 0 && now_ticks > dev->last_sample_tick) {
        float dt_ticks = (float)(now_ticks - dev->last_sample_tick);
        if (dt_ticks > 0.0f) {
            float dax = gx - dev->last_ax_g;
            float day = gy - dev->last_ay_g;
            float daz = gz - dev->last_az_g;

            float jerk_mag = sqrtf(dax*dax + day*day + daz*daz) / dt_ticks;
            dev->current_jerk_g_per_tick = jerk_mag;
        } else {
            dev->current_jerk_g_per_tick = 0.0f;
        }
    } else {
        // Insufficient history to compute jerk yet
        dev->current_jerk_g_per_tick = 0.0f;
    }

    dev->last_ax_g = gx;
    dev->last_ay_g = gy;
    dev->last_az_g = gz;

    dev->last_sample_tick = now_ticks;

    return 0;
}

void mc3479_task(MC3479 *dev, unsigned long now_ticks) {
    if (!dev) return;
    if (!dev->enabled) return;

    if ((now_ticks - dev->last_sample_tick) >= 20) {
        // Try to sample; if it fails, we leave the previous value intact
        if (mc3479_sample_now(dev, now_ticks) == 0) {
            // sample_now updates last_sample_tick
        } else {
            mc3479_log(dev, "mc3479: sample failed\n");
            // Advance the last_sample_tick anyway to avoid continuous retries
            dev->last_sample_tick = now_ticks;
        }
    }
}

float mc3479_get_magnitude(MC3479 *dev) {
    if (!dev) return 0.0f;
    return dev->current_magnitude_g;
}

float mc3479_get_jerk_magnitude(MC3479 *dev) {
    if (!dev) return 0.0f;
    return dev->current_jerk_g_per_tick;
}
