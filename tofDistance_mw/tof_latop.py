import argparse
import asyncio
from bleak import BleakScanner
import time
from statistics import mean
import numpy as np
import matplotlib.pyplot as plt

async def main():
    # rssiReading=[-51, -60, -64, -65, -65, -66, -75, -80, -80, -72]
    # Task 2.1 
    rssiReading=[]
    for dis in range(10):
        print(f"Place Nano at pos {dis}")
        devices = await BleakScanner.discover(return_adv=True)
        rssiReadingPos =[]
        for d, a in devices.values():
            if d.name == "Marko's Nano 33 BLE Sense":
                for i in range(5):
                    print(a.rssi)
                    rssiReadingPos.append(a.rssi)
                    time.sleep(0.5)
        rssiReading.append(mean(rssiReadingPos))
    print(rssiReading)
    distance = [0,10,20,30,40,50,60,70,80,90]
    A = np.vstack([distance, np.ones(len(distance))]).T
    m, c = np.linalg.lstsq(A, rssiReading, rcond=None)[0]
    print(f"{m}, {c}")

    #task 2.3
    _ = plt.plot(distance, rssiReading, 'o', label='Original data', markersize=10)
    newDistance = []
    for x in distance:
        newDistance.append((m*x) + c)
    _ = plt.plot(distance, newDistance, 'r', label='Fitted line')
    _ = plt.legend()
    plt.show()
    
    # Task 2.2
    print("showing distance based on rssi")
    while True:
        devices = await BleakScanner.discover(return_adv=True)
        for d, a in devices.values():
            if d.name == "Marko's Nano 33 BLE Sense":
                print((a.rssi - c)/m)

    
if __name__ == "__main__":
    asyncio.run(main())