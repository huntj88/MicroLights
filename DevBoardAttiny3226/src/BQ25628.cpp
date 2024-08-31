#include "BQ25628.hpp"

/**************************************************************************/
/*!
    @brief  Instantiates a new NAU7802 class
*/
/**************************************************************************/
BQ25628::BQ25628() {}

/**************************************************************************/
/*!
    @brief  Sets up the I2C connection and tests that the sensor was found.
    @param theWire Pointer to an I2C device we'll use to communicate
    default is Wire
    @return true if sensor was found, otherwise false.
*/
/**************************************************************************/
bool BQ25628::begin(TwoWire *theWire) {
  if (i2c_dev) {
    delete i2c_dev;
  }
  i2c_dev = new Adafruit_I2CDevice(I2CADDR_DEFAULT, theWire);

  /* Try to instantiate the I2C device. */
  if (!i2c_dev->begin()) {
    return false;
  }

  if (!reset())
    return false;
  if (!enable(true))
    return false;

  return true;
}

/**************************************************************************/
/*!
    @brief  Whether to have the sensor enabled and working or in power down mode
    @param  flag True to be in powered mode, False for power down mode
    @return False if something went wrong with I2C comms
*/
/**************************************************************************/
bool BQ25628::enable(bool flag) {
    // TODO
    return true;
}

/**************************************************************************/
/*!
    @brief Perform a soft reset
    @return False if there was any I2C comms error
*/
/**************************************************************************/
bool BQ25628::reset(void) {
    // TODO
    return true;
}