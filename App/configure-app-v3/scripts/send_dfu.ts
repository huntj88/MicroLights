import { parseArgs } from 'node:util';

import { WebUSB, WebUSBDevice } from 'usb';

const USB_VENDOR_ID = 0xcafe;
const USB_CONFIGURATION = 1;
const USB_INTERFACE = 0;
const USB_ENDPOINT_OUT = 1;

const encoder = new TextEncoder();
const dfuCommandPayload = encoder.encode('{"command":"dfu"}\n');
const dfuCommandBuffer = dfuCommandPayload.buffer.slice(
  dfuCommandPayload.byteOffset,
  dfuCommandPayload.byteOffset + dfuCommandPayload.byteLength,
);

const parseHexOrDecimal = (value: string, name: string) => {
  const parsed = Number(value);

  if (!Number.isInteger(parsed) || parsed < 0) {
    throw new Error(`Invalid ${name}: ${value}`);
  }

  return parsed;
};

const formatDeviceLabel = (device: WebUSBDevice) => {
  const vendorId = `0x${device.vendorId.toString(16).padStart(4, '0')}`;
  const productId = `0x${device.productId.toString(16).padStart(4, '0')}`;
  const serial = device.serialNumber ? ` serial=${device.serialNumber}` : '';
  const manufacturer = device.manufacturerName ?? 'Unknown manufacturer';
  const product = device.productName ?? 'Unknown product';

  return `${manufacturer} ${product} (${vendorId}:${productId}${serial})`;
};

const releaseAndClose = async (device: WebUSBDevice) => {
  if (!device.opened) {
    return;
  }

  await device.releaseInterface(USB_INTERFACE).catch(() => {
    return undefined;
  });
  await device.close().catch(() => {
    return undefined;
  });
};

const main = async () => {
  const { values } = parseArgs({
    options: {
      serial: { type: 'string' },
      'product-id': { type: 'string' },
      'vendor-id': { type: 'string' },
    },
  });

  const vendorId = values['vendor-id']
    ? parseHexOrDecimal(values['vendor-id'], 'vendor id')
    : USB_VENDOR_ID;
  const productId = values['product-id']
    ? parseHexOrDecimal(values['product-id'], 'product id')
    : undefined;

  const webusb = new WebUSB({ allowAllDevices: true });
  const devices = await webusb.getDevices();

  const matchingDevices = devices.filter((device): device is WebUSBDevice => {
    if (!(device instanceof WebUSBDevice)) {
      return false;
    }

    if (device.vendorId !== vendorId) {
      return false;
    }

    if (productId !== undefined && device.productId !== productId) {
      return false;
    }

    if (values.serial && device.serialNumber !== values.serial) {
      return false;
    }

    return true;
  });

  if (matchingDevices.length === 0) {
    const productFilter = productId !== undefined ? ` product=0x${productId.toString(16)}` : '';
    const serialFilter = values.serial ? ` serial=${values.serial}` : '';

    throw new Error(
      `No matching USB device found for vendor=0x${vendorId.toString(16)}${productFilter}${serialFilter}`,
    );
  }

  if (matchingDevices.length > 1 && !values.serial) {
    const details = matchingDevices.map(device => `- ${formatDeviceLabel(device)}`).join('\n');

    throw new Error(
      `Multiple matching USB devices found. Re-run with --serial <serial-number>.\n${details}`,
    );
  }

  const [device] = matchingDevices;

  console.log(`Connecting to ${formatDeviceLabel(device)}`);

  try {
    await device.open();

    if (!device.configuration) {
      await device.selectConfiguration(USB_CONFIGURATION);
    }

    await device.claimInterface(USB_INTERFACE);
    await device.transferOut(USB_ENDPOINT_OUT, dfuCommandBuffer);

    console.log('Sent DFU command successfully');
  } finally {
    await releaseAndClose(device);
  }
};

main().catch((error: unknown) => {
  const message = error instanceof Error ? error.message : String(error);
  console.error(`DFU command failed: ${message}`);
  process.exitCode = 1;
});
