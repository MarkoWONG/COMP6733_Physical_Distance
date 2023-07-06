import asyncio
from bleak import BleakScanner

m = 13.6
c = 63.9

async def main():
    global m
    global c
    print("Scanning for 5 seconds...")

    while (True):
        devices = await BleakScanner.discover(return_adv=True)
        for d,a in devices.values():
            # if "John" in str(d):
            print()
            print(d)
            print(a)
            rssi = max(0, int(str(a)[-3:-1]))
            dist = (rssi - c) / m
            print(f"RSSI: {rssi}, Distance: {dist}")


if __name__ == '__main__':
    asyncio.run(main())




'''
TODO:

1. Figure out how to send a packet (count of hops = integer) directly to another specific BLE device
2. Figure out how to take a timestamp from BLE device
3. Filter out BLE devices that are out of RSSI range, only test those that are reasonably close
4. 


'''