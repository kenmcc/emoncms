from  struct import *
import os
import sys
from Sensor import *
    
SENSORS = { 1:["COLLECTOR",None], 
            2:["WS:PTHum",PressureHumiditySensor],
            #3:["WS:WindRain",RainSensor],
            #10:["Kitchen", TempSensor],
            
            11:["SittingRoom", TempSensor],
            21:["SadhbhBedroom",TempSwitchSensor],
}
            
#print SENSORS

    
    
    
for x in range(0, 30):
    receivedData = {"temp_in_sensors":{}}
    data = pack('BBhhih', x, 10, 2170, 3315, 1, 9 )
    print len(data)
    sensorNode, length = unpack("BB", data[:2])
    payload = data[2:]
    if sensorNode in SENSORS:
        #print SENSORS[x]
        if SENSORS[sensorNode][1] is not None:
            sensorName = SENSORS[sensorNode][0]
            sensorFunc = SENSORS[sensorNode][1]
            proc_data = sensorFunc(sensorName, sensorNode).handleData(payload, receivedData)
            print receivedData

