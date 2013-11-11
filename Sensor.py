from struct import *
class Sensor(object):
    def __init__(self, name):
        self.name = name
       
    def handleData(self, data):
        return None

    
class UnKnown(Sensor):
    def handleData(self, data):
        pass

class HumiditySensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(HumiditySensor, self).__init__(name)
    
    def handleData(self, data):
        humidity, batt = unpack("hh", data[0:4])
        return {"Node":self.name,"Humidity":humidity, "Battery":batt}

class RainSensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(RainSensor, self).__init__(name)
    
    def handleData(self, data):
        rain,batt = unpack("hh", data[0:4])
        return {"Node":self.name, "Rain":rain, "Battery":batt}    

class TempSensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(TempSensor, self).__init__(name)
        
    def handleData(self, data):
        temp,batt = unpack("hh", data[0:4])
        return {"Node":self.name,"Temp":temp, "Battery":batt}
        
    
class TempSwitchSensor(TempSensor):
    def __init__(self, name, nodeId = None):
        super(TempSwitchSensor, self).__init__(name)
    def handleData(self, data):
        tempdata = super(TempSwitchSensor, self).handleData(data[0:4])
        tempdata['Switchstate'] = unpack("B", data[4])[0]
        return tempdata
