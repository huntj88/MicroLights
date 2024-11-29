#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Arduino.h>
#include <Wire.h>

/** Default I2C address. */
#define I2CADDR_DEFAULT 0x6A   
#define BQ25180_STAT0 0x00     
#define BQ25180_STAT1 0x01     
#define BQ25180_FLAG0 0x02     
#define BQ25180_VBAT_CTRL 0x3  
#define BQ25180_ICHG_CTRL 0x4  
#define BQ25180_CHARGECTRL0 0x5
#define BQ25180_CHARGECTRL1 0x6
#define BQ25180_IC_CTRL 0x7    
#define BQ25180_TMR_ILIM 0x8   
#define BQ25180_SHIP_RST 0x9   
#define BQ25180_SYS_REG 0xA    
#define BQ25180_TS_CONTROL 0xB 
#define BQ25180_MASK_ID 0xC    

class BQ25180 {
public:/*  */
    BQ25180();
    bool begin(TwoWire *theWire = &Wire);

    void dumpInfo();

private:
    Adafruit_I2CDevice *i2c = NULL;
};