import asyncio
import sys

from bleak import BleakClient
from bleak import BleakScanner
from bleak import discover
from bleak.backends.characteristic import BleakGATTCharacteristic

# UUIDs
RED_LED_UUID = '13012F01-F8C3-4F4A-A8F4-15CD926DA146'
GREEN_LED_UUID = '13012F02-F8C3-4F4A-A8F4-15CD926DA146'
BLUE_LED_UUID = '13012F03-F8C3-4F4A-A8F4-15CD926DA146'
distanceReading_UUID = "4A981236-1CC4-E7C1-C757-F1267DD021E8"

def handle_rx(_: BleakGATTCharacteristic, data: bytearray):
    print("received:", data.decode('utf-8'))

async def run():
    global  distanceReading_UUID

    print('Looking for "ContactTracing" Peripheral Device...')
    found = False
    devices = await BleakScanner.discover(return_adv=True)
    for d, a in devices.values():
        if d.name == "contactTracing":
            print('Found "ContactTracing" Peripheral Device')
            found = True

            # Connect to Peripheral
            async with BleakClient(d.address) as client:
                print(f'Connected to {d.address}')

                # await client.start_notify(distanceReading_UUID, handle_rx)
                
                # loop = asyncio.get_running_loop()
                custom_svc = client.services.get_service("4A981234-1CC4-E7C1-C757-F1267DD021E8")
                read_char = custom_svc.get_characteristic(distanceReading_UUID)
                while True:
                    # This waits until you type a line and press ENTER.
                    # data = await loop.run_in_executor(None, sys.stdin.buffer.readline)
                   
                    # # data will be empty on EOF (e.g. CTRL+D on *nix)
                    # if not data:
                    #     break

                    # await client.write_gatt_char(wrt_char, data)


                    await client.read_gatt_char(read_char)

    if not found:
        print('Could not find "ContactTracing" Peripheral Device')

                    
loop = asyncio.get_event_loop()
try:
    loop.run_until_complete(run())
except KeyboardInterrupt:
    print('\nReceived Keyboard Interrupt')
finally:
    print('Program finished')
        