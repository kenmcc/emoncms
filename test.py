from  struct import *
import sys
from Sensor import *



class UnKnown(Sensor):
    def handleData(self, data):
        pass

class HumiditySensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(HumiditySensor, self).__init__(name)
    
    def handleData(self, data):
        return {"Humidity":88}

class RainSensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(RainSensor, self).__init__(name)
    
    def handleData(self, data):
        rain,batt = unpack("hh", data[2:6])
        return {"Node":self.name, "Rain":rain, "Battery":batt}    

class TempSensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(TempSensor, self).__init__(name)
        
    def handleData(self, data):
        temp,batt = unpack("hh", data[2:6])
        return {"Node":self.name,"Temp":temp, "Battery":batt}
        
    
class TempSwitchSensor(TempSensor):
    def __init__(self, name, nodeId = None):
        super(TempSwitchSensor, self).__init__(name)
    def handleData(self, data):
        tempdata = super(TempSwitchSensor, self).handleData(data)
        tempdata['Switchstate'] = 1
        return tempdata
    
    
SENSORS = { 1:["COLLECTOR",None], 
            2:["WS:PTHum",HumiditySensor],
            3:["WS:WindRain",RainSensor],
            10:["Kitchen", TempSensor],
            11:["SittingRoom", TempSensor],
            21:["SadhbhBedroom",TempSwitchSensor],
}
            
#print SENSORS


for x in range(0, 30):
    data = pack('BBhhhh', x, 4, 1, 2, 3,4)
    if x in SENSORS:
        #print SENSORS[x]
        if SENSORS[x][1] is not None:
            sensorNode, length = unpack("BB", data[:2])
            sensorName = SENSORS[x][0]
            sensorFunc = SENSORS[x][1]
            print sensorFunc(sensorName, sensorNode).handleData(data[2:])

