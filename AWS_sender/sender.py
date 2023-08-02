
import asyncio
import platform
import sys
from bleak import BleakClient
#from bleak import BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic
import AWSIoTPythonSDK.MQTTLib as AWSIoTpyMQTT

myClient = AWSIoTpyMQTT.AWSIoTMQTTClient("6733FFA")
myClient.configureEndpoint('a2hh4760uae4jb-ats.iot.ap-southeast-2.amazonaws.com',8883)
myClient.configureCredentials("AmazonRootCA1.pem",
                              "d6ed1f6f6f4515616b3fc1c2a66df3d545037fbfbee856f8ebd959bf3164584f-private.pem.key",
                              "d6ed1f6f6f4515616b3fc1c2a66df3d545037fbfbee856f8ebd959bf3164584f-certificate.pem.crt")


ADDRESS = (
    ###############CHANGE TO TEST DEVICE ADDRESS################
    "D5:A7:E6:7B:AE:82"  
    if platform.system() != "Darwin"
    else "B9EA5233-37EF-4DD6-87A8-2A875E821C46" 
)


###############CHANGE TO TEST FILE UUID ################
custom_svc_uuid = "4A988008-1CC4-E7C1-C757-F1267DD021E8"
custom_wrt_char_uuid = "4A988118-1CC4-E7C1-C757-F1267DD021E8"
custom_read_char_uuid = "4A988228-1CC4-E7C1-C757-F1267DD021E8"

myClient.connect()
async def main():
    
    def handle_rx(_: BleakGATTCharacteristic, data: bytearray):
        print("received:", data)
        string = str(data.decode())
        myClient.publish("test/6733FFA",string,1)
        print("sent AWS:" + string)
            
        
    async with BleakClient(ADDRESS) as client:
        print(f"Connected: {client.is_connected}")
        await client.start_notify(custom_read_char_uuid, handle_rx)
        loop = asyncio.get_running_loop()
        custom_svc = client.services.get_service(custom_svc_uuid)
        wrt_char = custom_svc.get_characteristic(custom_wrt_char_uuid)
        read_char = custom_svc.get_characteristic(custom_read_char_uuid)
        while True:
            data = await loop.run_in_executor(None,sys.stdin.buffer.readline)
            if not data:
                break
            await client.write_gatt_char(wrt_char, data)
            print("sent:", data)
    


if __name__ == "__main__":
    asyncio.run(main())
    myClient.disconnect()
