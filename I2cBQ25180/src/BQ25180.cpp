#include "BQ25180.hpp"

BQ25180::BQ25180() {}

/**************************************************************************/
/*!
    @brief  Sets up the I2C connection and tests that the sensor was found.
    @param theWire Pointer to an I2C device we'll use to communicate
    default is Wire
    @return true if sensor was found, otherwise false.
*/
/**************************************************************************/
bool BQ25180::begin(TwoWire *theWire)
{
    if (i2c)
    {
        delete i2c;
    }
    i2c = new Adafruit_I2CDevice(I2CADDR_DEFAULT, theWire);

    /* Try to instantiate the I2C device. */
    if (!i2c->begin(true))
    {
        return false;
    }

    // if (!reset())
    //   return false;
    // if (!enable(true))
    //   return false;

    return true;
}

bool BQ25180::write(uint16_t reg_addr, uint8 flags)
{
    return Adafruit_I2CRegister(i2c, reg_addr).write(flags);
}

void dumpStat0Register(Adafruit_I2CDevice *i2c)
{
    // Print the header for the STAT0 Register dump
    Serial.println("STAT0 Register");

    // Read the STAT0 register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_STAT0);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Bitwise extraction for each bit field
    bool TS_OPEN_STAT = flags & 0b10000000;
    bool CHG_STAT_1 = flags & 0b01000000;
    bool CHG_STAT_0 = flags & 0b00100000;
    bool ILIM_ACTIVE_STAT = flags & 0b00010000;
    bool VDPPM_ACTIVE_STAT = flags & 0b00001000;
    bool VINDPM_ACTIVE_STAT = flags & 0b00000100;
    bool THERMREG_ACTIVE_STAT = flags & 0b00000010;
    bool VIN_PGOOD_STAT = flags & 0b00000001;

    // Description of each bit field in STAT0 register

    // TS_OPEN_STAT (Bit 7)
    Serial.println("Bit 7 TS_OPEN_STAT");
    if (TS_OPEN_STAT)
    {
        Serial.println(" 1 TS is open");
    }
    else
    {
        Serial.println(" 0 TS is connected");
    }

    // CHG_STAT (Bits 6-5)
    Serial.println("Bits 6-5 CHG_STAT");
    if (CHG_STAT_1)
    {
        if (CHG_STAT_0)
        {
            Serial.println(" 11 Charge termination done");
        }
        else
        {
            Serial.println(" 10 Fast charging");
        }
    }
    else
    {
        if (CHG_STAT_0)
        {
            Serial.println(" 01 Pre-charge in progress");
        }
        else
        {
            Serial.println(" 00 Not Charging");
        }
    }

    // ILIM_ACTIVE_STAT (Bit 4)
    Serial.println("Bit 4 ILIM_ACTIVE_STAT");
    if (ILIM_ACTIVE_STAT)
    {
        Serial.println(" 1 Input Current Limit Active");
    }
    else
    {
        Serial.println(" 0 Input Current Limit Not Active");
    }

    // VDPPM_ACTIVE_STAT (Bit 3)
    Serial.println("Bit 3 VDPPM_ACTIVE_STAT");
    if (VDPPM_ACTIVE_STAT)
    {
        Serial.println(" 1 VDPPM Mode Active");
    }
    else
    {
        Serial.println(" 0 VDPPM Mode Not Active");
    }

    // VINDPM_ACTIVE_STAT (Bit 2)
    Serial.println("Bit 2 VINDPM_ACTIVE_STAT");
    if (VINDPM_ACTIVE_STAT)
    {
        Serial.println(" 1 VINDPM Mode Active");
    }
    else
    {
        Serial.println(" 0 VINDPM Mode Not Active");
    }

    // THERMREG_ACTIVE_STAT (Bit 1)
    Serial.println("Bit 1 THERMREG_ACTIVE_STAT");
    if (THERMREG_ACTIVE_STAT)
    {
        Serial.println(" 1 Thermal Regulation Active");
    }
    else
    {
        Serial.println(" 0 Thermal Regulation Not Active");
    }

    // VIN_PGOOD_STAT (Bit 0)
    Serial.println("Bit 0 VIN_PGOOD_STAT");
    if (VIN_PGOOD_STAT)
    {
        Serial.println(" 1 VIN Power Good");
    }
    else
    {
        Serial.println(" 0 VIN Power Not Good");
    }
}

void dumpStat1Register(Adafruit_I2CDevice *i2c)
{
    // Print the header for the STAT1 Register dump
    Serial.println("STAT1 Register");

    // Read the STAT1 register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_STAT1);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Bitwise extraction for each bit field
    bool VIN_OVP_STAT = flags & 0b10000000;
    bool BUVLO_STAT = flags & 0b01000000;
    bool RESERVED = flags & 0b00100000;            // This is reserved, but we check it for completeness
    uint8 TS_STAT_1_0 = (flags & 0b00011000) >> 3; // Bits 4-3 for TS Status (2 bits)
    bool SAFETY_TMR_FAULT_FLAG = flags & 0b00000100;
    bool WAKE1_FLAG = flags & 0b00000010;
    bool WAKE2_FLAG = flags & 0b00000001;

    // Description of each bit field in STAT1 register

    // VIN_OVP_STAT (Bit 7)
    Serial.println("Bit 7 VIN_OVP_STAT");
    if (VIN_OVP_STAT)
    {
        Serial.println(" 1 VIN_OVP Fault Active");
    }
    else
    {
        Serial.println(" 0 VIN_OVP Fault Not Active");
    }

    // BUVLO_STAT (Bit 6)
    Serial.println("Bit 6 BUVLO_STAT");
    if (BUVLO_STAT)
    {
        Serial.println(" 1 Battery UVLO Status Active");
    }
    else
    {
        Serial.println(" 0 Battery UVLO Status Not Active");
    }

    // RESERVED (Bit 5)
    Serial.println("Bit 5 RESERVED");
    if (RESERVED)
    {
        Serial.println(" 1 Reserved bit, no valid status.");
    }
    else
    {
        Serial.println(" 0 Reserved bit, no valid status.");
    }

    // TS_STAT_1:0 (Bits 4-3)
    Serial.println("Bits 4-3 TS_STAT_1:0");
    switch (TS_STAT_1_0)
    {
    case 0b00:
        Serial.println(" 00 Normal");
        break;
    case 0b01:
        Serial.println(" 01 VTS < VHOT or VTS > VCOLD (charging suspended)");
        break;
    case 0b10:
        Serial.println(" 10 VCOOL < VTS < VCOLD (Charging current reduced by value set by TS_Registers)");
        break;
    case 0b11:
        Serial.println(" 11 VWARM > VTS > VHOT (Charging voltage reduced by value set by TS_Registers)");
        break;
    default:
        Serial.println(" Invalid TS Status");
        break;
    }

    // SAFETY_TMR_FAULT_FLAG (Bit 2)
    Serial.println("Bit 2 SAFETY_TMR_FAULT_FLAG");
    if (SAFETY_TMR_FAULT_FLAG)
    {
        Serial.println(" 1 Safety Timer Expired Fault Active");
    }
    else
    {
        Serial.println(" 0 Safety Timer Expired Fault Not Active");
    }

    // WAKE1_FLAG (Bit 1)
    Serial.println("Bit 1 WAKE1_FLAG");
    if (WAKE1_FLAG)
    {
        Serial.println(" 1 Wake 1 Timer Condition Met");
    }
    else
    {
        Serial.println(" 0 Wake 1 Timer Condition Not Met");
    }

    // WAKE2_FLAG (Bit 0)
    Serial.println("Bit 0 WAKE2_FLAG");
    if (WAKE2_FLAG)
    {
        Serial.println(" 1 Wake 2 Timer Condition Met");
    }
    else
    {
        Serial.println(" 0 Wake 2 Timer Condition Not Met");
    }
}

void dumpFlag0Register(Adafruit_I2CDevice *i2c)
{
    // Print the header for the FLAG0 Register dump
    Serial.println("FLAG0 Register");

    // Read the FLAG0 register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_FLAG0);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Bitwise extraction for each bit field
    bool TS_FAULT = flags & 0b10000000;
    bool ILIM_ACTIVE_FLAG = flags & 0b01000000;
    bool VDPPM_ACTIVE_FLAG = flags & 0b00100000;
    bool VINDPM_ACTIVE_FLAG = flags & 0b00010000;
    bool THERMREG_ACTIVE_FLAG = flags & 0b00001000;
    bool VIN_OVP_FAULT_FLAG = flags & 0b00000100;
    bool BUVLO_FAULT_FLAG = flags & 0b00000010;
    bool BAT_OCP_FAULT = flags & 0b00000001;

    // Description of each bit field in FLAG0 register

    // TS_FAULT (Bit 7)
    Serial.println("Bit 7 TS_FAULT");
    if (TS_FAULT)
    {
        Serial.println(" 1 TS Fault detected");
    }
    else
    {
        Serial.println(" 0 No TS Fault detected");
    }

    // ILIM_ACTIVE_FLAG (Bit 6)
    Serial.println("Bit 6 ILIM_ACTIVE_FLAG");
    if (ILIM_ACTIVE_FLAG)
    {
        Serial.println(" 1 ILIM Fault detected");
    }
    else
    {
        Serial.println(" 0 No ILIM Fault detected");
    }

    // VDPPM_ACTIVE_FLAG (Bit 5)
    Serial.println("Bit 5 VDPPM_ACTIVE_FLAG");
    if (VDPPM_ACTIVE_FLAG)
    {
        Serial.println(" 1 VDPPM Fault detected");
    }
    else
    {
        Serial.println(" 0 VDPPM Fault not detected");
    }

    // VINDPM_ACTIVE_FLAG (Bit 4)
    Serial.println("Bit 4 VINDPM_ACTIVE_FLAG");
    if (VINDPM_ACTIVE_FLAG)
    {
        Serial.println(" 1 VINDPM Fault detected");
    }
    else
    {
        Serial.println(" 0 VINDPM Fault not detected");
    }

    // THERMREG_ACTIVE_FLAG (Bit 3)
    Serial.println("Bit 3 THERMREG_ACTIVE_FLAG");
    if (THERMREG_ACTIVE_FLAG)
    {
        Serial.println(" 1 Thermal regulation has occurred");
    }
    else
    {
        Serial.println(" 0 No thermal regulation detected");
    }

    // VIN_OVP_FAULT_FLAG (Bit 2)
    Serial.println("Bit 2 VIN_OVP_FAULT_FLAG");
    if (VIN_OVP_FAULT_FLAG)
    {
        Serial.println(" 1 VIN_OVP Fault detected");
    }
    else
    {
        Serial.println(" 0 VIN_OVP Fault not detected");
    }

    // BUVLO_FAULT_FLAG (Bit 1)
    Serial.println("Bit 1 BUVLO_FAULT_FLAG");
    if (BUVLO_FAULT_FLAG)
    {
        Serial.println(" 1 Battery undervoltage fault detected");
    }
    else
    {
        Serial.println(" 0 Battery undervoltage fault not detected");
    }

    // BAT_OCP_FAULT (Bit 0)
    Serial.println("Bit 0 BAT_OCP_FAULT");
    if (BAT_OCP_FAULT)
    {
        Serial.println(" 1 Battery overcurrent condition detected");
    }
    else
    {
        Serial.println(" 0 Battery overcurrent condition not detected");
    }
}

void dumpVbatCtrlRegister(Adafruit_I2CDevice *i2c)
{
    // Print the header for the VBAT_CTRL Register dump
    Serial.println("VBAT_CTRL Register");

    // Read the VBAT_CTRL register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_VBAT_CTRL);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract the reserved bit (bit 7) and the VBATREG_6:0 field (bits 6-0)
    bool RESERVED = flags & 0b10000000;      // Reserved bit (Bit 7)
    uint8 VBATREG_CODE = flags & 0b01111111; // VBATREG_6:0 (Bits 6-0)

    // Calculate the battery regulation voltage (in mV)
    float VBATREG = 3500 + VBATREG_CODE * 10.0; // Formula: VBATREG = 3.5V + (VBATREG_CODE * 10mV)

    // Print the values and descriptions

    // RESERVED (Bit 7)
    Serial.println("Bit 7 RESERVED");
    if (RESERVED)
    {
        Serial.println(" 1 Reserved bit, not used.");
    }
    else
    {
        Serial.println(" 0 Reserved bit, not used.");
    }

    // VBATREG_6:0 (Bits 6-0)
    Serial.println("Bits 6-0 VBATREG_6:0");
    Serial.print("  VBATREG (Battery Regulation Voltage) = ");
    Serial.print(VBATREG);
    Serial.println(" mV");
    Serial.print("  (VBATREG_CODE = ");
    Serial.print(VBATREG_CODE);
    Serial.println(")");
}

void dumpIchgCtrlRegister(Adafruit_I2CDevice *i2c)
{
    // Print the header for the ICHG_CTRL Register dump
    Serial.println("ICHG_CTRL Register");

    // Read the ICHG_CTRL register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_ICHG_CTRL);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract the CHG_DIS bit (Bit 7) and the ICHG_6:0 field (Bits 6-0)
    bool CHG_DIS = flags & 0b10000000;   // CHG_DIS (Bit 7)
    uint8 ICHGCODE = flags & 0b01111111; // ICHG_6:0 (Bits 6-0)

    // Calculate the charging current (ICHG) based on the ICHGCODE
    uint16 ICHG;
    if (ICHGCODE <= 31)
    {
        // For ICHG ≤ 35mA, use the formula: ICHG = (ICHGCODE + 1) * 5mA
        ICHG = (ICHGCODE + 1) * 5;
    }
    else
    {
        // For ICHG > 35mA, use the formula: ICHG = 40 + ((ICHGCODE - 31) * 10)mA
        ICHG = 40 + ((ICHGCODE - 31) * 10);
    }

    // Print the values and descriptions

    // CHG_DIS (Bit 7)
    Serial.println("Bit 7 CHG_DIS");
    if (CHG_DIS)
    {
        Serial.println(" 1 Battery Charging Disabled");
    }
    else
    {
        Serial.println(" 0 Battery Charging Enabled");
    }

    // ICHG_6:0 (Bits 6-0)
    Serial.println("Bits 6-0 ICHG_6:0");
    Serial.print("  Charging Current (ICHG) = ");
    Serial.print(ICHG);
    Serial.println(" mA");
    Serial.print("  (ICHGCODE = ");
    Serial.print(ICHGCODE);
    Serial.println(")");
}

void dumpChargeCtrl0Register(Adafruit_I2CDevice *i2c)
{
    // Print the header for the CHARGECTRL0 Register dump
    Serial.println("CHARGECTRL0 Register");

    // Read the CHARGECTRL0 register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_CHARGECTRL0);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract each bit field
    bool RESERVED = flags & 0b10000000; // Bit 7
    bool IPRECHG = flags & 0b01000000;  // Bit 6
    uint8 ITERM = (flags >> 4) & 0b11;  // Bits 5-4
    uint8 VINDPM = (flags >> 2) & 0b11; // Bits 3-2
    uint8 THERM_REG = flags & 0b11;     // Bits 1-0

    // Precharge Current (IPRECHG)
    String prechargeDesc = (IPRECHG) ? "Precharge is Term" : "Precharge is 2x Term";

    // Termination Current (ITERM_1:0)
    String terminationDesc;
    switch (ITERM)
    {
    case 0b00:
        terminationDesc = "Disable";
        break;
    case 0b01:
        terminationDesc = "5% of ICHG";
        break;
    case 0b10:
        terminationDesc = "10% of ICHG";
        break;
    case 0b11:
        terminationDesc = "20% of ICHG";
        break;
    }

    // VINDPM Level (VINDPM_1:0)
    String vindpmDesc;
    switch (VINDPM)
    {
    case 0b00:
        vindpmDesc = "4.2V";
        break;
    case 0b01:
        vindpmDesc = "4.5V";
        break;
    case 0b10:
        vindpmDesc = "4.7V";
        break;
    case 0b11:
        vindpmDesc = "Disabled";
        break;
    }

    // Thermal Regulation Threshold (THERM_REG_1:0)
    String thermRegDesc;
    switch (THERM_REG)
    {
    case 0b00:
        thermRegDesc = "100°C";
        break;
    case 0b11:
        thermRegDesc = "Disabled";
        break;
    default:
        thermRegDesc = "Reserved"; // Technically not used but covered for completeness
        break;
    }

    // Print the values and descriptions

    // RESERVED (Bit 7)
    Serial.println("Bit 7 RESERVED");
    if (RESERVED)
    {
        Serial.println(" 1 Reserved bit, not used.");
    }
    else
    {
        Serial.println(" 0 Reserved bit, not used.");
    }

    // IPRECHG (Bit 6)
    Serial.println("Bit 6 IPRECHG");
    Serial.print("  ");
    Serial.println(prechargeDesc);

    // ITERM_1:0 (Bits 5-4)
    Serial.println("Bits 5-4 ITERM_1:0");
    Serial.print("  Termination current = ");
    Serial.println(terminationDesc);

    // VINDPM_1:0 (Bits 3-2)
    Serial.println("Bits 3-2 VINDPM_1:0");
    Serial.print("  VINDPM Level = ");
    Serial.println(vindpmDesc);

    // THERM_REG_1:0 (Bits 1-0)
    Serial.println("Bits 1-0 THERM_REG_1:0");
    Serial.print("  Thermal regulation threshold = ");
    Serial.println(thermRegDesc);
}

void dumpChargeCtrl1Register(Adafruit_I2CDevice *i2c)
{
    // Print the header for the CHARGECTRL1 Register dump
    Serial.println("CHARGECTRL1 Register");

    // Read the CHARGECTRL1 register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_CHARGECTRL1);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract each bit field
    uint8 IBAT_OCP = (flags >> 6) & 0b11;          // Bits 7-6
    uint8 BUVLO = (flags >> 3) & 0b111;            // Bits 5-3
    bool CHG_STATUS_INT_MASK = (flags >> 2) & 0b1; // Bit 2
    bool ILIM_INT_MASK = (flags >> 1) & 0b1;       // Bit 1
    bool VDPM_INT_MASK = flags & 0b1;              // Bit 0

    // Battery Discharge Current Limit (IBAT_OCP_1:0)
    String dischargeCurrentDesc;
    switch (IBAT_OCP)
    {
    case 0b00:
        dischargeCurrentDesc = "500mA";
        break;
    case 0b01:
        dischargeCurrentDesc = "1000mA";
        break;
    case 0b10:
        dischargeCurrentDesc = "1500mA";
        break;
    case 0b11:
        dischargeCurrentDesc = "Disabled";
        break;
    }

    // Battery Undervoltage LockOut (BUVLO_2:0)
    String undervoltageThresholdDesc;
    switch (BUVLO)
    {
    case 0b000:
    case 0b001:
    case 0b010:
        undervoltageThresholdDesc = "3.0V";
        break;
    case 0b011:
        undervoltageThresholdDesc = "2.8V";
        break;
    case 0b100:
        undervoltageThresholdDesc = "2.6V";
        break;
    case 0b101:
        undervoltageThresholdDesc = "2.4V";
        break;
    case 0b110:
        undervoltageThresholdDesc = "2.2V";
        break;
    case 0b111:
        undervoltageThresholdDesc = "2.0V";
        break;
    }

    // Mask Charging Status Interrupt (CHG_STATUS_INT_MASK)
    String chargingStatusInterruptDesc = (CHG_STATUS_INT_MASK) ? "Masked" : "Enabled";

    // Mask ILIM Fault Interrupt (ILIM_INT_MASK)
    String ilimInterruptDesc = (ILIM_INT_MASK) ? "Masked" : "Enabled";

    // Mask VINDPM and VDPPM Interrupt (VDPM_INT_MASK)
    String vdpmInterruptDesc = (VDPM_INT_MASK) ? "Masked" : "Enabled";

    // Print the values and descriptions

    // IBAT_OCP_1:0 (Bits 7-6)
    Serial.println("Bits 7-6 IBAT_OCP_1:0");
    Serial.print("  Battery Discharge Current Limit = ");
    Serial.println(dischargeCurrentDesc);

    // BUVLO_2:0 (Bits 5-3)
    Serial.println("Bits 5-3 BUVLO_2:0");
    Serial.print("  Battery Undervoltage LockOut Falling Threshold = ");
    Serial.println(undervoltageThresholdDesc);

    // CHG_STATUS_INT_MASK (Bit 2)
    Serial.println("Bit 2 CHG_STATUS_INT_MASK");
    Serial.print("  Mask Charging Status Interrupt = ");
    Serial.println(chargingStatusInterruptDesc);

    // ILIM_INT_MASK (Bit 1)
    Serial.println("Bit 1 ILIM_INT_MASK");
    Serial.print("  Mask ILIM Fault Interrupt = ");
    Serial.println(ilimInterruptDesc);

    // VDPM_INT_MASK (Bit 0)
    Serial.println("Bit 0 VDPM_INT_MASK");
    Serial.print("  Mask VINDPM and VDPPM Interrupt = ");
    Serial.println(vdpmInterruptDesc);
}

void dumpICCtrlRegister(Adafruit_I2CDevice *i2c)
{
    // Print the header for the IC_CTRL Register dump
    Serial.println("IC_CTRL Register");

    // Read the IC_CTRL register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_IC_CTRL);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract each bit field
    bool TS_EN = (flags >> 7) & 0b1;          // Bit 7
    bool VLOWV_SEL = (flags >> 6) & 0b1;      // Bit 6
    bool VRCH_0 = (flags >> 5) & 0b1;         // Bit 5
    bool XTMR_EN = (flags >> 4) & 0b1;        // Bit 4
    uint8 SAFETY_TIMER = (flags >> 2) & 0b11; // Bits 3-2
    uint8 WATCHDOG_SEL = flags & 0b11;        // Bits 1-0

    // TS Auto Function Enable (TS_EN)
    String tsAutoFunctionDesc = TS_EN ? "Enabled" : "Disabled";

    // Precharge Voltage Threshold (VLOWV_SEL)
    String prechargeVoltageDesc = VLOWV_SEL ? "2.8V" : "3.0V";

    // Recharge Voltage Threshold (VRCH_0)
    String rechargeVoltageDesc = VRCH_0 ? "200mV" : "100mV";

    // Timer Slow Enable (2XTMR_EN)
    String timerSlowDesc = XTMR_EN ? "Enabled (2x slower)" : "Disabled";

    // Fast Charge Timer (SAFETY_TIMER_1:0)
    String safetyTimerDesc;
    switch (SAFETY_TIMER)
    {
    case 0b00:
        safetyTimerDesc = "3 hours";
        break;
    case 0b01:
        safetyTimerDesc = "6 hours";
        break;
    case 0b10:
        safetyTimerDesc = "12 hours";
        break;
    case 0b11:
        safetyTimerDesc = "Disabled";
        break;
    }

    // Watchdog Timer Selection (WATCHDOG_SEL_1:0)
    String watchdogDesc;
    switch (WATCHDOG_SEL)
    {
    case 0b00:
        watchdogDesc = "160s default register values";
        break;
    case 0b01:
        watchdogDesc = "160s HW_RESET";
        break;
    case 0b10:
        watchdogDesc = "40s HW_RESET";
        break;
    case 0b11:
        watchdogDesc = "Disabled";
        break;
    }

    // Print the values and descriptions

    // TS Auto Function (TS_EN)
    Serial.println("Bit 7 TS_EN");
    Serial.print("  TS Auto Function = ");
    Serial.println(tsAutoFunctionDesc);

    // Precharge Voltage Threshold (VLOWV_SEL)
    Serial.println("Bit 6 VLOWV_SEL");
    Serial.print("  Precharge Voltage Threshold = ");
    Serial.println(prechargeVoltageDesc);

    // Recharge Voltage Threshold (VRCH_0)
    Serial.println("Bit 5 VRCH_0");
    Serial.print("  Recharge Voltage Threshold = ");
    Serial.println(rechargeVoltageDesc);

    // Timer Slow Enable (2XTMR_EN)
    Serial.println("Bit 4 2XTMR_EN");
    Serial.print("  Timer Slow = ");
    Serial.println(timerSlowDesc);

    // Fast Charge Timer (SAFETY_TIMER_1:0)
    Serial.println("Bits 3-2 SAFETY_TIMER_1:0");
    Serial.print("  Fast Charge Timer = ");
    Serial.println(safetyTimerDesc);

    // Watchdog Timer Selection (WATCHDOG_SEL_1:0)
    Serial.println("Bits 1-0 WATCHDOG_SEL_1:0");
    Serial.print("  Watchdog Timer Selection = ");
    Serial.println(watchdogDesc);
}

void dumpTMR_ILIMRegister(Adafruit_I2CDevice *i2c)
{
    // Print the header for the TMR_ILIM Register dump
    Serial.println("TMR_ILIM Register");

    // Read the TMR_ILIM register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_TMR_ILIM);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract each bit field
    uint8 MR_LPRESS = (flags >> 6) & 0b11;  // Bits 7-6
    bool MR_RESET_VIN = (flags >> 5) & 0b1; // Bit 5
    uint8 AUTOWAKE = (flags >> 3) & 0b11;   // Bits 4-3
    uint8 ILIM = flags & 0b111;             // Bits 2-0

    // Push Button Long Press Duration Timer (MR_LPRESS_1:0)
    String longPressDurationDesc;
    switch (MR_LPRESS)
    {
    case 0b00:
        longPressDurationDesc = "5s";
        break;
    case 0b01:
        longPressDurationDesc = "10s";
        break;
    case 0b10:
        longPressDurationDesc = "15s";
        break;
    case 0b11:
        longPressDurationDesc = "20s";
        break;
    }

    // Hardware Reset Condition (MR_RESET_VIN)
    String resetConditionDesc = MR_RESET_VIN ? "Reset sent when long press duration is met and VIN_Powergood" : "Reset sent when long press duration is met";

    // Auto Wake Up Timer Restart (AUTOWAKE_1:0)
    String autoWakeDurationDesc;
    switch (AUTOWAKE)
    {
    case 0b00:
        autoWakeDurationDesc = "0.5s";
        break;
    case 0b01:
        autoWakeDurationDesc = "1s";
        break;
    case 0b10:
        autoWakeDurationDesc = "2s";
        break;
    case 0b11:
        autoWakeDurationDesc = "4s";
        break;
    }

    // Input Current Limit Setting (ILIM_2:0)
    String inputCurrentLimitDesc;
    switch (ILIM)
    {
    case 0b000:
        inputCurrentLimitDesc = "50mA";
        break;
    case 0b001:
        inputCurrentLimitDesc = "100mA (max.)";
        break;
    case 0b010:
        inputCurrentLimitDesc = "200mA";
        break;
    case 0b011:
        inputCurrentLimitDesc = "300mA";
        break;
    case 0b100:
        inputCurrentLimitDesc = "400mA";
        break;
    case 0b101:
        inputCurrentLimitDesc = "500mA (max.)";
        break;
    case 0b110:
        inputCurrentLimitDesc = "700mA";
        break;
    case 0b111:
        inputCurrentLimitDesc = "1100mA";
        break;
    }

    // Print the values and descriptions

    // Push Button Long Press Duration Timer (MR_LPRESS_1:0)
    Serial.println("Bits 7-6 MR_LPRESS_1:0");
    Serial.print("  Push Button Long Press Duration = ");
    Serial.println(longPressDurationDesc);

    // Hardware Reset Condition (MR_RESET_VIN)
    Serial.println("Bit 5 MR_RESET_VIN");
    Serial.print("  Hardware Reset Condition = ");
    Serial.println(resetConditionDesc);

    // Auto Wake Up Timer Restart (AUTOWAKE_1:0)
    Serial.println("Bits 4-3 AUTOWAKE_1:0");
    Serial.print("  Auto Wake Up Timer Restart = ");
    Serial.println(autoWakeDurationDesc);

    // Input Current Limit Setting (ILIM_2:0)
    Serial.println("Bits 2-0 ILIM_2:0");
    Serial.print("  Input Current Limit = ");
    Serial.println(inputCurrentLimitDesc);
}

void dumpSHIP_RSTRegister(Adafruit_I2CDevice *i2c)
{
    // Print the header for the SHIP_RST Register dump
    Serial.println("SHIP_RST Register");

    // Read the SHIP_RST register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_SHIP_RST);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract each bit field
    bool REG_RST = (flags >> 7) & 0b1;            // Bit 7
    uint8 EN_RST_SHIP = (flags >> 5) & 0b11;      // Bits 6-5
    uint8 PB_LPRESS_ACTION = (flags >> 3) & 0b11; // Bits 4-3
    bool WAKE1_TMR = (flags >> 2) & 0b1;          // Bit 2
    bool WAKE2_TMR = (flags >> 1) & 0b1;          // Bit 1
    bool EN_PUSH = flags & 0b1;                   // Bit 0

    // Software Reset (REG_RST)
    String resetDesc = REG_RST ? "Software Reset Triggered" : "No Reset";

    // Shipmode Enable and Hardware Reset (EN_RST_SHIP_1:0)
    String shipModeDesc;
    switch (EN_RST_SHIP)
    {
    case 0b00:
        shipModeDesc = "Do nothing";
        break;
    case 0b01:
        shipModeDesc = "Enable shutdown mode with wake on adapter insert only";
        break;
    case 0b10:
        shipModeDesc = "Enable shipmode with wake on button press or adapter insert";
        break;
    case 0b11:
        shipModeDesc = "Hardware Reset";
        break;
    }

    // Pushbutton Long Press Action (PB_LPRESS_ACTION_1:0)
    String pbLongPressDesc;
    switch (PB_LPRESS_ACTION)
    {
    case 0b00:
        pbLongPressDesc = "Do nothing";
        break;
    case 0b01:
        pbLongPressDesc = "Hardware Reset";
        break;
    case 0b10:
        pbLongPressDesc = "Enable shipmode";
        break;
    case 0b11:
        pbLongPressDesc = "Enable shutdown mode";
        break;
    }

    // Wake 1 Timer Setting (WAKE1_TMR)
    String wake1Desc = WAKE1_TMR ? "1s" : "300ms";

    // Wake 2 Timer Setting (WAKE2_TMR)
    String wake2Desc = WAKE2_TMR ? "3s" : "2s";

    // Enable Push Button and Reset Function on Battery Only (EN_PUSH)
    String pushButtonDesc = EN_PUSH ? "Enabled" : "Disabled";

    // Print the values and descriptions

    // Software Reset (REG_RST)
    Serial.println("Bit 7 REG_RST");
    Serial.print("  Software Reset = ");
    Serial.println(resetDesc);

    // Shipmode Enable and Hardware Reset (EN_RST_SHIP_1:0)
    Serial.println("Bits 6-5 EN_RST_SHIP_1:0");
    Serial.print("  Shipmode Enable and Hardware Reset = ");
    Serial.println(shipModeDesc);

    // Pushbutton Long Press Action (PB_LPRESS_ACTION_1:0)
    Serial.println("Bits 4-3 PB_LPRESS_ACTION_1:0");
    Serial.print("  Pushbutton Long Press Action = ");
    Serial.println(pbLongPressDesc);

    // Wake 1 Timer Setting (WAKE1_TMR)
    Serial.println("Bit 2 WAKE1_TMR");
    Serial.print("  Wake 1 Timer = ");
    Serial.println(wake1Desc);

    // Wake 2 Timer Setting (WAKE2_TMR)
    Serial.println("Bit 1 WAKE2_TMR");
    Serial.print("  Wake 2 Timer = ");
    Serial.println(wake2Desc);

    // Enable Push Button and Reset Function on Battery Only (EN_PUSH)
    Serial.println("Bit 0 EN_PUSH");
    Serial.print("  Push Button and Reset Function on Battery = ");
    Serial.println(pushButtonDesc);
}

void dumpSYS_REGRegister(Adafruit_I2CDevice *i2c)
{
    // Print the header for the SYS_REG Register dump
    Serial.println("SYS_REG Register");

    // Read the SYS_REG register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_SYS_REG);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract each bit field
    uint8 SYS_REG_CTRL = (flags >> 5) & 0b111;     // Bits 7-5
    bool RESERVED = (flags >> 4) & 0b1;            // Bit 4
    uint8 SYS_MODE = (flags >> 2) & 0b11;          // Bits 3-2
    bool WATCHDOG_15S_ENABLE = (flags >> 1) & 0b1; // Bit 1
    bool VDPPM_DIS = flags & 0b1;                  // Bit 0

    // SYS Regulation Voltage (SYS_REG_CTRL_2:0)
    String sysVoltageDesc;
    switch (SYS_REG_CTRL)
    {
    case 0b000:
        sysVoltageDesc = "Battery Tracking Mode";
        break;
    case 0b001:
        sysVoltageDesc = "4.4V";
        break;
    case 0b010:
        sysVoltageDesc = "4.5V";
        break;
    case 0b011:
        sysVoltageDesc = "4.6V";
        break;
    case 0b100:
        sysVoltageDesc = "4.7V";
        break;
    case 0b101:
        sysVoltageDesc = "4.8V";
        break;
    case 0b110:
        sysVoltageDesc = "4.9V";
        break;
    case 0b111:
        sysVoltageDesc = "Pass-Through (VSYS = VIN)";
        break;
    }

    // SYS Power Source Mode (SYS_MODE_1:0)
    String sysModeDesc;
    switch (SYS_MODE)
    {
    case 0b00:
        sysModeDesc = "SYS powered from VIN if present or VBAT";
        break;
    case 0b01:
        sysModeDesc = "SYS powered from VBAT only, even if VIN present";
        break;
    case 0b10:
        sysModeDesc = "SYS disconnected and left floating";
        break;
    case 0b11:
        sysModeDesc = "SYS disconnected with pulldown";
        break;
    }

    // Watchdog 15s Enable (WATCHDOG_15S_ENABLE)
    String watchdogDesc = WATCHDOG_15S_ENABLE ? "Enabled: HW reset after 15s if no I2C transaction" : "Disabled";

    // VDPPM Disable (VDPPM_DIS)
    String vdppmDesc = VDPPM_DIS ? "Disabled" : "Enabled";

    // Print the values and descriptions

    // SYS Regulation Voltage (SYS_REG_CTRL_2:0)
    Serial.println("Bits 7-5 SYS_REG_CTRL_2:0");
    Serial.print("  SYS Regulation Voltage = ");
    Serial.println(sysVoltageDesc);

    // Reserved (RESERVED)
    Serial.println("Bit 4 RESERVED");
    Serial.print("  Reserved = ");
    Serial.println(RESERVED ? "1" : "0");

    // SYS Power Source Mode (SYS_MODE_1:0)
    Serial.println("Bits 3-2 SYS_MODE_1:0");
    Serial.print("  SYS Power Source Mode = ");
    Serial.println(sysModeDesc);

    // Watchdog 15s Enable (WATCHDOG_15S_ENABLE)
    Serial.println("Bit 1 WATCHDOG_15S_ENABLE");
    Serial.print("  Watchdog 15s Enable = ");
    Serial.println(watchdogDesc);

    // VDPPM Disable (VDPPM_DIS)
    Serial.println("Bit 0 VDPPM_DIS");
    Serial.print("  VDPPM = ");
    Serial.println(vdppmDesc);
}

void dumpTS_CONTROLRegister(Adafruit_I2CDevice *i2c)
{
    // Print the header for the TS_CONTROL Register dump
    Serial.println("TS_CONTROL Register");

    // Read the TS_CONTROL register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_TS_CONTROL);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract each bit field
    uint8 TS_HOT = (flags >> 6) & 0b11;  // Bits 7-6
    uint8 TS_COLD = (flags >> 4) & 0b11; // Bits 5-4
    bool TS_WARM = (flags >> 3) & 0b1;   // Bit 3
    bool TS_COOL = (flags >> 2) & 0b1;   // Bit 2
    bool TS_ICHG = (flags >> 1) & 0b1;   // Bit 1
    bool TS_VRCG = flags & 0b1;          // Bit 0

    // TS Hot Threshold (TS_HOT)
    String tsHotDesc;
    switch (TS_HOT)
    {
    case 0b00:
        tsHotDesc = "60C (default)";
        break;
    case 0b01:
        tsHotDesc = "65C";
        break;
    case 0b10:
        tsHotDesc = "50C";
        break;
    case 0b11:
        tsHotDesc = "45C";
        break;
    }

    // TS Cold Threshold (TS_COLD)
    String tsColdDesc;
    switch (TS_COLD)
    {
    case 0b00:
        tsColdDesc = "0C (default)";
        break;
    case 0b01:
        tsColdDesc = "3C";
        break;
    case 0b10:
        tsColdDesc = "5C";
        break;
    case 0b11:
        tsColdDesc = "-3C";
        break;
    }

    // TS Warm Threshold (TS_WARM)
    String tsWarmDesc = TS_WARM ? "Disabled" : "45C (default)";

    // TS Cool Threshold (TS_COOL)
    String tsCoolDesc = TS_COOL ? "Disabled" : "10C (default)";

    // TS ICHG (TS_ICHG)
    String tsIchgDesc = TS_ICHG ? "0.2 * ICHG" : "0.5 * ICHG";

    // TS VRCG (TS_VRCG)
    String tsVrcgDesc = TS_VRCG ? "VBATREG - 200mV" : "VBATREG - 100mV";

    // Print the values and descriptions

    // TS Hot Threshold (TS_HOT)
    Serial.println("Bits 7-6 TS_HOT");
    Serial.print("  TS Hot Threshold = ");
    Serial.println(tsHotDesc);

    // TS Cold Threshold (TS_COLD)
    Serial.println("Bits 5-4 TS_COLD");
    Serial.print("  TS Cold Threshold = ");
    Serial.println(tsColdDesc);

    // TS Warm Threshold (TS_WARM)
    Serial.println("Bit 3 TS_WARM");
    Serial.print("  TS Warm Threshold = ");
    Serial.println(tsWarmDesc);

    // TS Cool Threshold (TS_COOL)
    Serial.println("Bit 2 TS_COOL");
    Serial.print("  TS Cool Threshold = ");
    Serial.println(tsCoolDesc);

    // TS ICHG (TS_ICHG)
    Serial.println("Bit 1 TS_ICHG");
    Serial.print("  Fast charge current = ");
    Serial.println(tsIchgDesc);

    // TS VRCG (TS_VRCG)
    Serial.println("Bit 0 TS_VRCG");
    Serial.print("  Reduced target battery voltage = ");
    Serial.println(tsVrcgDesc);
}

void dumpMASK_IDRegister(Adafruit_I2CDevice *i2c)
{
    // Print the header for the MASK_ID Register dump
    Serial.println("MASK_ID Register");

    // Read the MASK_ID register into the flags variable
    Adafruit_I2CRegister r = Adafruit_I2CRegister(i2c, BQ25180_MASK_ID);
    uint8 flags; // 8-bit register
    r.read(&flags);

    // Extract each bit field
    bool TS_INT_MASK = (flags >> 7) & 0b1;   // Bit 7
    bool TREG_INT_MASK = (flags >> 6) & 0b1; // Bit 6
    bool BAT_INT_MASK = (flags >> 5) & 0b1;  // Bit 5
    bool PG_INT_MASK = (flags >> 4) & 0b1;   // Bit 4
    uint8 deviceID = flags & 0b1111;         // Bits 3-0 (Device ID)

    // TS Interrupt Mask (TS_INT_MASK)
    String tsIntMaskDesc = TS_INT_MASK ? "Mask TS Interrupt" : "Enable TS Interrupt";

    // TREG Interrupt Mask (TREG_INT_MASK)
    String tregIntMaskDesc = TREG_INT_MASK ? "Mask TREG Interrupt" : "Enable TREG Interrupt";

    // Battery Overcurrent and Undervoltage Mask (BAT_INT_MASK)
    String batIntMaskDesc = BAT_INT_MASK ? "Mask BOCP and BUVLO Interrupt" : "Enable BOCP and BUVLO Interrupt";

    // PG and VINOVP Interrupt Mask (PG_INT_MASK)
    String pgIntMaskDesc = PG_INT_MASK ? "Mask PG and VINOVP Interrupt" : "Enable PG and VINOVP Interrupt";

    // Device ID (Device_ID)
    String deviceIDDesc;
    switch (deviceID)
    {
    case 0b0000:
        deviceIDDesc = "BQ25180";
        break;
    default:
        deviceIDDesc = "Unknown Device";
        break;
    }

    // Print the values and descriptions

    // TS Interrupt Mask (TS_INT_MASK)
    Serial.println("Bit 7 TS_INT_MASK");
    Serial.print("  TS Interrupt Mask = ");
    Serial.println(tsIntMaskDesc);

    // TREG Interrupt Mask (TREG_INT_MASK)
    Serial.println("Bit 6 TREG_INT_MASK");
    Serial.print("  TREG Interrupt Mask = ");
    Serial.println(tregIntMaskDesc);

    // Battery Overcurrent and Undervoltage Mask (BAT_INT_MASK)
    Serial.println("Bit 5 BAT_INT_MASK");
    Serial.print("  Battery Overcurrent and Undervoltage Mask = ");
    Serial.println(batIntMaskDesc);

    // PG and VINOVP Interrupt Mask (PG_INT_MASK)
    Serial.println("Bit 4 PG_INT_MASK");
    Serial.print("  PG and VINOVP Interrupt Mask = ");
    Serial.println(pgIntMaskDesc);

    // Device ID (Device_ID)
    Serial.println("Bits 3-0 Device_ID");
    Serial.print("  Device ID = ");
    Serial.println(deviceIDDesc);
}

void BQ25180::dumpInfo()
{
    // Serial.println();
    // Adafruit_I2CRegister(i2c, BQ25180_STAT0).println();
    // Adafruit_I2CRegister(i2c, BQ25180_STAT1).println();
    // Adafruit_I2CRegister(i2c, BQ25180_FLAG0).println();
    // Adafruit_I2CRegister(i2c, BQ25180_VBAT_CTRL).println();
    // Adafruit_I2CRegister(i2c, BQ25180_ICHG_CTRL).println();
    // Adafruit_I2CRegister(i2c, BQ25180_CHARGECTRL0).println();
    // Adafruit_I2CRegister(i2c, BQ25180_CHARGECTRL1).println();
    // Adafruit_I2CRegister(i2c, BQ25180_IC_CTRL).println();
    // Adafruit_I2CRegister(i2c, BQ25180_TMR_ILIM).println();
    // Adafruit_I2CRegister(i2c, BQ25180_SHIP_RST).println();
    // Adafruit_I2CRegister(i2c, BQ25180_SYS_REG).println();
    // Adafruit_I2CRegister(i2c, BQ25180_TS_CONTROL).println();
    // Adafruit_I2CRegister(i2c, BQ25180_MASK_ID).println();

    Serial.println();
    dumpStat0Register(i2c);
    Serial.println();
    dumpStat1Register(i2c);
    Serial.println();
    dumpFlag0Register(i2c);
    Serial.println();
    dumpVbatCtrlRegister(i2c);
    Serial.println();
    dumpIchgCtrlRegister(i2c);
    Serial.println();
    dumpChargeCtrl0Register(i2c);
    Serial.println();
    dumpChargeCtrl1Register(i2c);
    Serial.println();
    dumpICCtrlRegister(i2c);
    Serial.println();
    dumpTMR_ILIMRegister(i2c);
    Serial.println();
    dumpSHIP_RSTRegister(i2c);
    Serial.println();
    dumpSYS_REGRegister(i2c);
    Serial.println();
    dumpTS_CONTROLRegister(i2c);
    Serial.println();
    dumpMASK_IDRegister(i2c);
}

// SYS_REG_CTRL_2: 3b000 = Battery Tracking Mode, more efficient use of power?
// VBATREG_6: 4.4v, (3.5v) + (90 * 10mV), 90 = 0b1011010
// VBATREG_6: 4.3v, (3.5v) + (80 * 10mV), 80 = 0b1010000

// maybe start at like 50, instead of 70?
// ICHG_6: 70 milliamp, 40 + ((34) - 31) * 10, 34 = 0b100010
// ICHG_6: 50 milliamp, 40 + ((32) - 31) * 10, 32 = 0b100000

// TS_EN: 1b0 = TS auto function disabled (Only charge control is disabled. TS monitoring is enabled)

// WATCHDOG_SEL_1, disable at some point to save battery? probably not

// EN_RST_SHIP_1:
//  2b01 = Enable shutdown mode with wake on adapter insert only            // for when battery is dead?
//  2b10 = Enable shipmode with wake on button press or adapter insert

// PB_LPRESS_ACTION_1: 2b10 = Enable shipmode
