export interface BQ25180Registers {
  stat0?: string;
  stat1?: string;
  flag0?: string;
  vbat_ctrl?: string;
  ichg_ctrl?: string;
  chargectrl0?: string;
  chargectrl1?: string;
  ic_ctrl?: string;
  tmr_ilim?: string;
  ship_rst?: string;
  sys_reg?: string;
  ts_control?: string;
  mask_id?: string;
}

export interface DecodedRegisterField {
  name: string;
  value: string;
  description: string;
}

export interface DecodedRegister {
  name: string;
  raw: string;
  fields: DecodedRegisterField[];
}

const binToDec = (bin: string): number => parseInt(bin, 2);

export const decodeBQ25180Registers = (registers: BQ25180Registers): DecodedRegister[] => {
  const decoded: DecodedRegister[] = [];

  if (registers.ichg_ctrl) {
    const val = registers.ichg_ctrl;
    const ichgCode = binToDec(val.slice(1, 8));
    let ichgCurrent = 0;

    if (ichgCode <= 31) {
      ichgCurrent = ichgCode + 5;
    } else {
      ichgCurrent = 40 + (ichgCode - 31) * 10;
    }

    decoded.push({
      name: 'ICHG_CTRL',
      raw: val,
      fields: [
        {
          name: 'CHG_DIS',
          value: val[0],
          description: val[0] === '0' ? 'Enabled' : 'Disabled',
        },
        {
          name: 'ICHG',
          value: val.slice(1, 8),
          description: `${ichgCurrent}mA`,
        },
      ],
    });
  }

  if (registers.chargectrl0) {
    const val = registers.chargectrl0;
    decoded.push({
      name: 'CHARGECTRL0',
      raw: val,
      fields: [
        {
          name: 'IPRECHG',
          value: val[1],
          description: val[1] === '0' ? '2x Term' : 'Term',
        },
        {
          name: 'ITERM',
          value: val.slice(2, 4),
          description:
            {
              '00': 'Disable',
              '01': '5% of ICHG',
              '10': '10% of ICHG',
              '11': '20% of ICHG',
            }[val.slice(2, 4)] || 'Unknown',
        },
        {
          name: 'VINDPM',
          value: val.slice(4, 6),
          description:
            {
              '00': '4.2 V',
              '01': '4.5 V',
              '10': '4.7 V',
              '11': 'Disabled',
            }[val.slice(4, 6)] || 'Unknown',
        },
        {
          name: 'THERM_REG',
          value: val.slice(6, 8),
          description:
            {
              '00': '100C',
              '11': 'Disabled',
            }[val.slice(6, 8)] || 'Unknown',
        },
      ],
    });
  }

  if (registers.chargectrl1) {
    const val = registers.chargectrl1;
    decoded.push({
      name: 'CHARGECTRL1',
      raw: val,
      fields: [
        {
          name: 'IBAT_OCP',
          value: val.slice(0, 2),
          description:
            {
              '00': '500mA',
              '01': '1000mA',
              '10': '1500mA',
              '11': 'Disabled',
            }[val.slice(0, 2)] || 'Unknown',
        },
        {
          name: 'BUVLO',
          value: val.slice(2, 5),
          description:
            {
              '000': '3.0V',
              '001': '3.0V',
              '010': '3.0V',
              '011': '2.8V',
              '100': '2.6V',
              '101': '2.4V',
              '110': '2.2V',
              '111': '2.0V',
            }[val.slice(2, 5)] || 'Unknown',
        },
        {
          name: 'CHG_STATUS_INT_MASK',
          value: val[5],
          description: val[5] === '0' ? 'Enable' : 'Mask',
        },
        {
          name: 'ILIM_INT_MASK',
          value: val[6],
          description: val[6] === '0' ? 'Enable' : 'Mask',
        },
        {
          name: 'VDPM_INT_MASK',
          value: val[7],
          description: val[7] === '0' ? 'Enable' : 'Mask',
        },
      ],
    });
  }

  if (registers.ic_ctrl) {
    const val = registers.ic_ctrl;
    decoded.push({
      name: 'IC_CTRL',
      raw: val,
      fields: [
        {
          name: 'TS_EN',
          value: val[0],
          description: val[0] === '1' ? 'Enabled' : 'Disabled',
        },
        {
          name: 'VLOWV_SEL',
          value: val[1],
          description: val[1] === '0' ? '3V' : '2.8V',
        },
        {
          name: 'VRCH_0',
          value: val[2],
          description: val[2] === '0' ? '100mV' : '200mV',
        },
        {
          name: '2XTMR_EN',
          value: val[3],
          description: val[3] === '0' ? 'Not slowed' : 'Slowed 2x',
        },
        {
          name: 'SAFETY_TIMER',
          value: val.slice(4, 6),
          description:
            {
              '00': '3 hour',
              '01': '6 hour',
              '10': '12 hour',
              '11': 'Disable',
            }[val.slice(4, 6)] || 'Unknown',
        },
        {
          name: 'WATCHDOG_SEL',
          value: val.slice(6, 8),
          description:
            {
              '00': '160s',
              '01': '160s HW_RESET',
              '10': '40s HW_RESET',
              '11': 'Disable',
            }[val.slice(6, 8)] || 'Unknown',
        },
      ],
    });
  }

  if (registers.tmr_ilim) {
    const val = registers.tmr_ilim;
    decoded.push({
      name: 'TMR_ILIM',
      raw: val,
      fields: [
        {
          name: 'MR_LPRESS',
          value: val.slice(0, 2),
          description:
            {
              '00': '5s',
              '01': '10s',
              '10': '15s',
              '11': '20s',
            }[val.slice(0, 2)] || 'Unknown',
        },
        {
          name: 'MR_RESET_VIN',
          value: val[2],
          description: val[2] === '0' ? 'Reset on duration' : 'Reset on duration + VIN_PG',
        },
        {
          name: 'AUTOWAKE',
          value: val.slice(3, 5),
          description:
            {
              '00': '0.5s',
              '01': '1s',
              '10': '2s',
              '11': '4s',
            }[val.slice(3, 5)] || 'Unknown',
        },
        {
          name: 'ILIM',
          value: val.slice(5, 8),
          description:
            {
              '000': '50mA',
              '001': '100mA',
              '010': '200mA',
              '011': '300mA',
              '100': '400mA',
              '101': '500mA',
              '110': '700mA',
              '111': '1100mA',
            }[val.slice(5, 8)] || 'Unknown',
        },
      ],
    });
  }

  if (registers.ship_rst) {
    const val = registers.ship_rst;
    decoded.push({
      name: 'SHIP_RST',
      raw: val,
      fields: [
        {
          name: 'REG_RST',
          value: val[0],
          description: val[0] === '1' ? 'Software Reset' : 'Do nothing',
        },
        {
          name: 'EN_RST_SHIP',
          value: val.slice(1, 3),
          description:
            {
              '00': 'Do nothing',
              '01': 'Shutdown (wake on adapter)',
              '10': 'Shipmode (wake on button/adapter)',
              '11': 'Hardware Reset',
            }[val.slice(1, 3)] || 'Unknown',
        },
        {
          name: 'PB_LPRESS_ACTION',
          value: val.slice(3, 5),
          description:
            {
              '00': 'Do nothing',
              '01': 'Hardware Reset',
              '10': 'Enable shipmode',
              '11': 'Enable shutdown mode',
            }[val.slice(3, 5)] || 'Unknown',
        },
        {
          name: 'WAKE1_TMR',
          value: val[5],
          description: val[5] === '0' ? '300ms' : '1s',
        },
        {
          name: 'WAKE2_TMR',
          value: val[6],
          description: val[6] === '0' ? '2s' : '3s',
        },
        {
          name: 'EN_PUSH',
          value: val[7],
          description: val[7] === '1' ? 'Enable' : 'Disable',
        },
      ],
    });
  }

  if (registers.sys_reg) {
    const val = registers.sys_reg;
    decoded.push({
      name: 'SYS_REG',
      raw: val,
      fields: [
        {
          name: 'SYS_REG_CTRL',
          value: val.slice(0, 3),
          description:
            {
              '000': 'Battery Tracking',
              '001': '4.4V',
              '010': '4.5V',
              '011': '4.6V',
              '100': '4.7V',
              '101': '4.8V',
              '110': '4.9V',
              '111': 'Pass-Through',
            }[val.slice(0, 3)] || 'Unknown',
        },
        {
          name: 'SYS_MODE',
          value: val.slice(4, 6),
          description:
            {
              '00': 'SYS from VIN/VBAT',
              '01': 'SYS from VBAT only',
              '10': 'SYS floating',
              '11': 'SYS pulldown',
            }[val.slice(4, 6)] || 'Unknown',
        },
        {
          name: 'WATCHDOG_15S_ENABLE',
          value: val[6],
          description: val[6] === '1' ? 'Enabled' : 'Disabled',
        },
        {
          name: 'VDPPM_DIS',
          value: val[7],
          description: val[7] === '1' ? 'Disabled' : 'Enabled',
        },
      ],
    });
  }

  if (registers.ts_control) {
    const val = registers.ts_control;
    decoded.push({
      name: 'TS_CONTROL',
      raw: val,
      fields: [
        {
          name: 'TS_HOT',
          value: val.slice(0, 2),
          description:
            {
              '00': '60C',
              '01': '65C',
              '10': '50C',
              '11': '45C',
            }[val.slice(0, 2)] || 'Unknown',
        },
        {
          name: 'TS_COLD',
          value: val.slice(2, 4),
          description:
            {
              '00': '0C',
              '01': '3C',
              '10': '5C',
              '11': '-3C',
            }[val.slice(2, 4)] || 'Unknown',
        },
        {
          name: 'TS_WARM',
          value: val[4],
          description: val[4] === '0' ? '45C' : 'Disabled',
        },
        {
          name: 'TS_COOL',
          value: val[5],
          description: val[5] === '0' ? '10C' : 'Disabled',
        },
        {
          name: 'TS_ICHG',
          value: val[6],
          description: val[6] === '0' ? '0.5*ICHG' : '0.2*ICHG',
        },
        {
          name: 'TS_VRCG',
          value: val[7],
          description: val[7] === '0' ? 'VBATREG -100mV' : 'VBATREG -200mV',
        },
      ],
    });
  }

  if (registers.mask_id) {
    const val = registers.mask_id;
    decoded.push({
      name: 'MASK_ID',
      raw: val,
      fields: [
        {
          name: 'TS_INT_MASK',
          value: val[0],
          description: val[0] === '1' ? 'Mask' : 'Enable',
        },
        {
          name: 'TREG_INT_MASK',
          value: val[1],
          description: val[1] === '1' ? 'Mask' : 'Enable',
        },
        {
          name: 'BAT_INT_MASK',
          value: val[2],
          description: val[2] === '1' ? 'Mask' : 'Enable',
        },
        {
          name: 'PG_INT_MASK',
          value: val[3],
          description: val[3] === '1' ? 'Mask' : 'Enable',
        },
        {
          name: 'Device_ID',
          value: val.slice(4, 8),
          description: val.slice(4, 8) === '0000' ? 'BQ25180' : 'Unknown',
        },
      ],
    });
  }

  if (registers.vbat_ctrl) {
    const val = registers.vbat_ctrl;
    const vbatCode = binToDec(val.slice(1, 8));
    const vbatVoltage = 3.5 + vbatCode * 0.01;

    decoded.push({
      name: 'VBAT_CTRL',
      raw: val,
      fields: [
        {
          name: 'VBATREG',
          value: val.slice(1, 8),
          description: `${vbatVoltage.toFixed(2)}V`,
        },
      ],
    });
  }

  if (registers.flag0) {
    const val = registers.flag0;
    decoded.push({
      name: 'FLAG0',
      raw: val,
      fields: [
        {
          name: 'TS_FAULT',
          value: val[0],
          description: val[0] === '1' ? 'Fault detected' : 'Normal',
        },
        {
          name: 'ILIM_ACTIVE',
          value: val[1],
          description: val[1] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'VDPPM_ACTIVE',
          value: val[2],
          description: val[2] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'VINDPM_ACTIVE',
          value: val[3],
          description: val[3] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'THERMREG_ACTIVE',
          value: val[4],
          description: val[4] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'VIN_OVP_FAULT',
          value: val[5],
          description: val[5] === '1' ? 'Fault detected' : 'Normal',
        },
        {
          name: 'BUVLO_FAULT',
          value: val[6],
          description: val[6] === '1' ? 'Fault detected' : 'Normal',
        },
        {
          name: 'BAT_OCP_FAULT',
          value: val[7],
          description: val[7] === '1' ? 'Fault detected' : 'Normal',
        },
      ],
    });
  }

  if (registers.stat1) {
    const val = registers.stat1;
    decoded.push({
      name: 'STAT1',
      raw: val,
      fields: [
        {
          name: 'VIN_OVP_STAT',
          value: val[0],
          description: val[0] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'BUVLO_STAT',
          value: val[1],
          description: val[1] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'TS_STAT',
          value: val.slice(3, 5),
          description:
            {
              '00': 'Normal',
              '01': 'VTS < VHOT or VTS > VCOLD',
              '10': 'VCOOL < VTS < VCOLD',
              '11': 'VWARM > VTS > VHOT',
            }[val.slice(3, 5)] || 'Unknown',
        },
        {
          name: 'SAFETY_TMR_FAULT',
          value: val[5],
          description: val[5] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'WAKE1_FLAG',
          value: val[6],
          description: val[6] === '1' ? 'Met Condition' : 'Not Met',
        },
        {
          name: 'WAKE2_FLAG',
          value: val[7],
          description: val[7] === '1' ? 'Met Condition' : 'Not Met',
        },
      ],
    });
  }

  if (registers.stat0) {
    const val = registers.stat0;
    decoded.push({
      name: 'STAT0',
      raw: val,
      fields: [
        {
          name: 'TS_OPEN_STAT',
          value: val[0],
          description: val[0] === '1' ? 'Open' : 'Normal',
        },
        {
          name: 'CHG_STAT',
          value: val.slice(1, 3),
          description:
            {
              '00': 'Not Charging',
              '01': 'Constant Current (CC)',
              '10': 'Constant Voltage (CV)',
              '11': 'Charge Done / Disabled',
            }[val.slice(1, 3)] || 'Unknown',
        },
        {
          name: 'ILIM_ACTIVE_STAT',
          value: val[3],
          description: val[3] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'VDPPM_ACTIVE_STAT',
          value: val[4],
          description: val[4] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'VINDPM_ACTIVE_STAT',
          value: val[5],
          description: val[5] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'THERMREG_ACTIVE_STAT',
          value: val[6],
          description: val[6] === '1' ? 'Active' : 'Normal',
        },
        {
          name: 'VIN_PGOOD_STAT',
          value: val[7],
          description: val[7] === '1' ? 'Power Good' : 'Power Not Good',
        },
      ],
    });
  }

  return decoded;
};
