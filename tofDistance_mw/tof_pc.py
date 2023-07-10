import argparse
import asyncio
import platform
import sys
from bleak import BleakClient
from bleak import BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

ADDRESS = (
    "66:22:9D:37:7E:1D" # Change to your device's address if on Linux/Windows
    if platform.system() != "Darwin"
    else "B9EA5233-37EF-4DD6-87A8-2A875E821C46" # Change to  your device's address if on macOS
)
custom_svc_uuid = "4A981234-1CC4-E7C1-C757-F1267DD021E8"
custom_wrt_char_uuid = "4A981235-1CC4-E7C1-C757-F1267DD021E8"
custom_read_char_uuid = "4A981236-1CC4-E7C1-C757-F1267DD021E8"
async def main():
    def handle_rx(_: BleakGATTCharacteristic, data: bytearray):
        # print("received:", data)
        
        if data == b'r\r\n':
            print("RED LED")
        elif data == b'g\r\n':
            print("Green LED")
        elif data == b'b\r\n':
            print("blue LED")
        elif data == b'p\r\n':
            print("humidity") 
        elif data == b't\r\n':
            print("Temperature")   
        elif data == b'i\r\n':
            print("Accelerometer")   
        else:
            print("Unknown input")  
            
    async with BleakClient(ADDRESS) as client:
        print(f"Connected: {client.is_connected}")
        await client.start_notify(custom_read_char_uuid, handle_rx)
        print("Connected, start typing and press ENTER...")
        loop = asyncio.get_running_loop()
        custom_svc = client.services.get_service(custom_svc_uuid)
        wrt_char = custom_svc.get_characteristic(custom_wrt_char_uuid)
        while True:
            # This waits until you type a line and press ENTER.
            # A real terminal program might put stdin in raw mode so that things
            # like CTRL+C get passed to the remote device.
            data = await loop.run_in_executor(None, sys.stdin.buffer.readline)
            # data will be empty on EOF (e.g. CTRL+D on *nix)
            if not data:
                break
            await client.write_gatt_char(wrt_char, data)
            # print("sent:", data)

if __name__ == "__main__":
    asyncio.run(main())