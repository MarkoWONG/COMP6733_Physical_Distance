import asyncio
from bleak import BleakScanner, BleakClient
from time import time_ns
import sys

REPS = 10

# m = 13.6
# c = 63.9

# async def main():
#     global m
#     global c
#     print("Scanning for 5 seconds...")

#     while (True):
#         devices = await BleakScanner.discover(return_adv=True)
#         for d,a in devices.values():
#             # if "John" in str(d):
#             print()
#             print(d)
#             print(a)
#             rssi = max(0, int(str(a)[-3:-1]))
#             dist = (rssi - c) / m
#             print(f"RSSI: {rssi}, Distance: {dist}")

ADDRESS = "C9DD2D84-ED90-B462-4B07-A1BBF5D1C24F"

custom_svc_uuid = "4A981234-1CC4-E7C1-C757-F1267DD021E8"
custom_wrt_char_uuid = "4A981235-1CC4-E7C1-C757-F1267DD021E8"
custom_read_char_uuid = "4A981236-1CC4-E7C1-C757-F1267DD021E8"

async def main():


    async with BleakClient(ADDRESS) as client:
        print(f"Connected: {client.is_connected}")
        print("Connected, start typing and press ENTER...")
        loop = asyncio.get_running_loop()
        custom_svc = client.services.get_service(custom_svc_uuid)
        wrt_char = custom_svc.get_characteristic(custom_wrt_char_uuid)
        # read_char = custom_svc.get_characteristic(custom_read_char_uuid)
        data = await loop.run_in_executor(None, sys.stdin.buffer.readline)

        start = time_ns()
        for _ in range(REPS):
            await client.write_gatt_char(wrt_char, data, response=True)
            print("Looped")

        finish = time_ns()
        elapsed = finish - start
        print(elapsed)


if __name__ == '__main__':
    asyncio.run(main())




'''
TODO:

1. Figure out how to send a packet (count of hops = integer) directly to another specific BLE device
2. Figure out how to take a timestamp from BLE device
3. Filter out BLE devices that are out of RSSI range, only test those that are reasonably close
4. Set a timer so that the check of nearby devices occurs at a certain frequency (maybe 1 - 5 seconds)
5. 


'''