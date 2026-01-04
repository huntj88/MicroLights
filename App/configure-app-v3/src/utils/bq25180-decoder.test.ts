import { describe, expect, it } from 'vitest';

import { decodeBQ25180Registers } from './bq25180-decoder';

describe('decodeBQ25180Registers', () => {
  it('should decode ICHG_CTRL correctly', () => {
    // Test case 1: ICHGCODE <= 31 (0x1F)
    // 0x05 = 00000101 -> CHG_DIS=0, ICHG=5 -> 5 + 5 = 10mA
    let result = decodeBQ25180Registers({ ichg_ctrl: '00000101' });
    let reg = result.find(r => r.name === 'ICHG_CTRL');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'CHG_DIS', value: '0', description: 'Enabled' },
      { name: 'ICHG', value: '0000101', description: '10mA' },
    ]);

    // Test case 2: ICHGCODE > 31 (0x1F)
    // 0x22 = 00100010 -> CHG_DIS=0, ICHG=34 -> 40 + (34-31)*10 = 70mA
    result = decodeBQ25180Registers({ ichg_ctrl: '00100010' });
    reg = result.find(r => r.name === 'ICHG_CTRL');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'CHG_DIS', value: '0', description: 'Enabled' },
      { name: 'ICHG', value: '0100010', description: '70mA' },
    ]);

    // Test case 3: Charging Disabled
    // 0x85 = 10000101 -> CHG_DIS=1, ICHG=5 -> 10mA
    result = decodeBQ25180Registers({ ichg_ctrl: '10000101' });
    reg = result.find(r => r.name === 'ICHG_CTRL');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'CHG_DIS', value: '1', description: 'Disabled' },
      { name: 'ICHG', value: '0000101', description: '10mA' },
    ]);
  });

  it('should decode CHARGECTRL0 correctly', () => {
    const result = decodeBQ25180Registers({ chargectrl0: '00101100' });
    const reg = result.find(r => r.name === 'CHARGECTRL0');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'IPRECHG', value: '0', description: '2x Term' },
      { name: 'ITERM', value: '10', description: '10% of ICHG' },
      { name: 'VINDPM', value: '11', description: 'Disabled' },
      { name: 'THERM_REG', value: '00', description: '100C' },
    ]);
  });

  it('should decode CHARGECTRL1 correctly', () => {
    const result = decodeBQ25180Registers({ chargectrl1: '00000011' });
    const reg = result.find(r => r.name === 'CHARGECTRL1');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'IBAT_OCP', value: '00', description: '500mA' },
      { name: 'BUVLO', value: '000', description: '3.0V' },
      { name: 'CHG_STATUS_INT_MASK', value: '0', description: 'Enable' },
      { name: 'ILIM_INT_MASK', value: '1', description: 'Mask' },
      { name: 'VDPM_INT_MASK', value: '1', description: 'Mask' },
    ]);
  });

  it('should decode IC_CTRL correctly', () => {
    const result = decodeBQ25180Registers({ ic_ctrl: '00000100' });
    const reg = result.find(r => r.name === 'IC_CTRL');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'TS_EN', value: '0', description: 'Disabled' },
      { name: 'VLOWV_SEL', value: '0', description: '3V' },
      { name: 'VRCH_0', value: '0', description: '100mV' },
      { name: '2XTMR_EN', value: '0', description: 'Not slowed' },
      { name: 'SAFETY_TIMER', value: '01', description: '6 hour' },
      { name: 'WATCHDOG_SEL', value: '00', description: '160s' },
    ]);
  });

  it('should decode TMR_ILIM correctly', () => {
    const result = decodeBQ25180Registers({ tmr_ilim: '01001101' });
    const reg = result.find(r => r.name === 'TMR_ILIM');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'MR_LPRESS', value: '01', description: '10s' },
      { name: 'MR_RESET_VIN', value: '0', description: 'Reset on duration' },
      { name: 'AUTOWAKE', value: '01', description: '1s' },
      { name: 'ILIM', value: '101', description: '500mA' },
    ]);
  });

  it('should decode SHIP_RST correctly', () => {
    const result = decodeBQ25180Registers({ ship_rst: '00010001' });
    const reg = result.find(r => r.name === 'SHIP_RST');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'REG_RST', value: '0', description: 'Do nothing' },
      { name: 'EN_RST_SHIP', value: '00', description: 'Do nothing' },
      { name: 'PB_LPRESS_ACTION', value: '10', description: 'Enable shipmode' },
      { name: 'WAKE1_TMR', value: '0', description: '300ms' },
      { name: 'WAKE2_TMR', value: '0', description: '2s' },
      { name: 'EN_PUSH', value: '1', description: 'Enable' },
    ]);
  });

  it('should decode VBAT_CTRL correctly', () => {
    // Test case 1: Default value (0x46)
    // 0x46 = 01000110 -> VBATREG=70 -> 3.5 + 70*0.01 = 4.20V
    let result = decodeBQ25180Registers({ vbat_ctrl: '01000110' });
    let reg = result.find(r => r.name === 'VBAT_CTRL');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([{ name: 'VBATREG', value: '1000110', description: '4.20V' }]);

    // Test case 2: Minimum value (0x00)
    // 0x00 = 00000000 -> VBATREG=0 -> 3.5 + 0 = 3.50V
    result = decodeBQ25180Registers({ vbat_ctrl: '00000000' });
    reg = result.find(r => r.name === 'VBAT_CTRL');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([{ name: 'VBATREG', value: '0000000', description: '3.50V' }]);

    // Test case 3: Maximum value (0x7F)
    // 0x7F = 01111111 -> VBATREG=127 -> 3.5 + 1.27 = 4.77V (Note: Datasheet says max programmable is 4.65V, but formula yields higher)
    // Let's test a value that results in 4.65V -> (4.65 - 3.5) / 0.01 = 115 -> 0x73 -> 01110011
    result = decodeBQ25180Registers({ vbat_ctrl: '01110011' });
    reg = result.find(r => r.name === 'VBAT_CTRL');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([{ name: 'VBATREG', value: '1110011', description: '4.65V' }]);
  });

  it('should decode FLAG0 correctly', () => {
    // Test case 1: No faults (0x00)
    let result = decodeBQ25180Registers({ flag0: '00000000' });
    let reg = result.find(r => r.name === 'FLAG0');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'TS_FAULT', value: '0', description: 'Normal' },
      { name: 'ILIM_ACTIVE', value: '0', description: 'Normal' },
      { name: 'VDPPM_ACTIVE', value: '0', description: 'Normal' },
      { name: 'VINDPM_ACTIVE', value: '0', description: 'Normal' },
      { name: 'THERMREG_ACTIVE', value: '0', description: 'Normal' },
      { name: 'VIN_OVP_FAULT', value: '0', description: 'Normal' },
      { name: 'BUVLO_FAULT', value: '0', description: 'Normal' },
      { name: 'BAT_OCP_FAULT', value: '0', description: 'Normal' },
    ]);

    // Test case 2: All faults active (0xFF)
    result = decodeBQ25180Registers({ flag0: '11111111' });
    reg = result.find(r => r.name === 'FLAG0');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'TS_FAULT', value: '1', description: 'Fault detected' },
      { name: 'ILIM_ACTIVE', value: '1', description: 'Active' },
      { name: 'VDPPM_ACTIVE', value: '1', description: 'Active' },
      { name: 'VINDPM_ACTIVE', value: '1', description: 'Active' },
      { name: 'THERMREG_ACTIVE', value: '1', description: 'Active' },
      { name: 'VIN_OVP_FAULT', value: '1', description: 'Fault detected' },
      { name: 'BUVLO_FAULT', value: '1', description: 'Fault detected' },
      { name: 'BAT_OCP_FAULT', value: '1', description: 'Fault detected' },
    ]);
  });

  it('should decode STAT1 correctly', () => {
    // Test case 1: Normal operation (0x00)
    let result = decodeBQ25180Registers({ stat1: '00000000' });
    let reg = result.find(r => r.name === 'STAT1');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'VIN_OVP_STAT', value: '0', description: 'Normal' },
      { name: 'BUVLO_STAT', value: '0', description: 'Normal' },
      { name: 'TS_STAT', value: '00', description: 'Normal' },
      { name: 'SAFETY_TMR_FAULT', value: '0', description: 'Normal' },
      { name: 'WAKE1_FLAG', value: '0', description: 'Not Met' },
      { name: 'WAKE2_FLAG', value: '0', description: 'Not Met' },
    ]);

    // Test case 2: All flags active (0xFF)
    // Note: Bit 5 is reserved, so we ignore it in the check or expect it to be handled if we decoded it
    result = decodeBQ25180Registers({ stat1: '11111111' });
    reg = result.find(r => r.name === 'STAT1');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'VIN_OVP_STAT', value: '1', description: 'Active' },
      { name: 'BUVLO_STAT', value: '1', description: 'Active' },
      { name: 'TS_STAT', value: '11', description: 'VWARM > VTS > VHOT' },
      { name: 'SAFETY_TMR_FAULT', value: '1', description: 'Active' },
      { name: 'WAKE1_FLAG', value: '1', description: 'Met Condition' },
      { name: 'WAKE2_FLAG', value: '1', description: 'Met Condition' },
    ]);
  });

  it('should decode STAT0 correctly', () => {
    // Test case 1: Normal operation, Not Charging, Power Good (0x01)
    // 0x01 = 00000001 -> TS_OPEN=0, CHG_STAT=00, ILIM=0, VDPPM=0, VINDPM=0, THERMREG=0, VIN_PG=1
    let result = decodeBQ25180Registers({ stat0: '00000001' });
    let reg = result.find(r => r.name === 'STAT0');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'TS_OPEN_STAT', value: '0', description: 'Normal' },
      { name: 'CHG_STAT', value: '00', description: 'Not Charging' },
      { name: 'ILIM_ACTIVE_STAT', value: '0', description: 'Normal' },
      { name: 'VDPPM_ACTIVE_STAT', value: '0', description: 'Normal' },
      { name: 'VINDPM_ACTIVE_STAT', value: '0', description: 'Normal' },
      { name: 'THERMREG_ACTIVE_STAT', value: '0', description: 'Normal' },
      { name: 'VIN_PGOOD_STAT', value: '1', description: 'Power Good' },
    ]);

    // Test case 2: Charging (CC), All Active (0x7F)
    // 0x7F = 01111111 -> TS_OPEN=0, CHG_STAT=11, ILIM=1, VDPPM=1, VINDPM=1, THERMREG=1, VIN_PG=1
    result = decodeBQ25180Registers({ stat0: '01111111' });
    reg = result.find(r => r.name === 'STAT0');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'TS_OPEN_STAT', value: '0', description: 'Normal' },
      { name: 'CHG_STAT', value: '11', description: 'Charge Done / Disabled' },
      { name: 'ILIM_ACTIVE_STAT', value: '1', description: 'Active' },
      { name: 'VDPPM_ACTIVE_STAT', value: '1', description: 'Active' },
      { name: 'VINDPM_ACTIVE_STAT', value: '1', description: 'Active' },
      { name: 'THERMREG_ACTIVE_STAT', value: '1', description: 'Active' },
      { name: 'VIN_PGOOD_STAT', value: '1', description: 'Power Good' },
    ]);

    // Test case 3: TS Open (0x80)
    result = decodeBQ25180Registers({ stat0: '10000000' });
    reg = result.find(r => r.name === 'STAT0');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'TS_OPEN_STAT', value: '1', description: 'Open' },
      { name: 'CHG_STAT', value: '00', description: 'Not Charging' },
      { name: 'ILIM_ACTIVE_STAT', value: '0', description: 'Normal' },
      { name: 'VDPPM_ACTIVE_STAT', value: '0', description: 'Normal' },
      { name: 'VINDPM_ACTIVE_STAT', value: '0', description: 'Normal' },
      { name: 'THERMREG_ACTIVE_STAT', value: '0', description: 'Normal' },
      { name: 'VIN_PGOOD_STAT', value: '0', description: 'Power Not Good' },
    ]);
  });

  it('should decode SYS_REG correctly', () => {
    const result = decodeBQ25180Registers({ sys_reg: '00000010' });
    const reg = result.find(r => r.name === 'SYS_REG');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'SYS_REG_CTRL', value: '000', description: 'Battery Tracking' },
      { name: 'SYS_MODE', value: '00', description: 'SYS from VIN/VBAT' },
      { name: 'WATCHDOG_15S_ENABLE', value: '1', description: 'Enabled' },
      { name: 'VDPPM_DIS', value: '0', description: 'Enabled' },
    ]);
  });

  it('should decode TS_CONTROL correctly', () => {
    const result = decodeBQ25180Registers({ ts_control: '00000000' });
    const reg = result.find(r => r.name === 'TS_CONTROL');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'TS_HOT', value: '00', description: '60C' },
      { name: 'TS_COLD', value: '00', description: '0C' },
      { name: 'TS_WARM', value: '0', description: '45C' },
      { name: 'TS_COOL', value: '0', description: '10C' },
      { name: 'TS_ICHG', value: '0', description: '0.5*ICHG' },
      { name: 'TS_VRCG', value: '0', description: 'VBATREG -100mV' },
    ]);
  });

  it('should decode MASK_ID correctly', () => {
    const result = decodeBQ25180Registers({ mask_id: '00000000' });
    const reg = result.find(r => r.name === 'MASK_ID');
    expect(reg).toBeDefined();
    expect(reg?.fields).toEqual([
      { name: 'TS_INT_MASK', value: '0', description: 'Enable' },
      { name: 'TREG_INT_MASK', value: '0', description: 'Enable' },
      { name: 'BAT_INT_MASK', value: '0', description: 'Enable' },
      { name: 'PG_INT_MASK', value: '0', description: 'Enable' },
      { name: 'Device_ID', value: '0000', description: 'BQ25180' },
    ]);
  });
});
