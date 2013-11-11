from struct import *

def checkBattery(node, battV):
  if battV < 2750 and node not in notifiedLowBatteries:
    sendEmail("Low Battery Warning", "Battery on node " + node + " is low: " + battV, "ken.mccullagh@gmail.com")
    notifiedLowBatteries.append(node)

def validateTemp(temp):
  if temp > -2000 and temp < 4000:
    return True
  return False

class Sensor(object):
    def __init__(self, name, nodeId = None):
        self.name = name
	self.nodeId = nodeId
       
    def handleData(self, data):
        return None

    
class UnKnown(Sensor):
    def handleData(self, data):
        pass

class HumiditySensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(HumiditySensor, self).__init__(name, nodeId)
    
    def handleData(self, data):
        humidity, batt = unpack("hh", data[0:4])
        checkBattery(self.nodeId, batt)
        return {"Node":self.name,"Humidity":humidity, "Battery":batt}

class PressureSensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(PressureSensor, self).__init__(name)
    
    def handleData(self, data):
        temp, batt, pressure, humidity = unpack("hhii", data[0:12])
        checkBattery(self.nodeId, batt)
        if validateTemp(temp) != True:
            temp = None
        else:
	    temp = float(temp)/10.0
	humidity = float(humidity)/100.0
        return {"Node":self.name,"Pressure":pressure, "Humidity":humidity,"Temp":temp, "Battery":batt}


class RainSensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(RainSensor, self).__init__(name)
    
    def handleData(self, data):
        rain,batt = unpack("hh", data[0:4])
        checkBattery(self.nodeId, batt)
        return {"Node":self.name, "Rain":rain, "Battery":batt}    

class TempSensor(Sensor):
    def __init__(self, name, nodeId = None):
        self.nodeId = nodeId
	super(TempSensor, self).__init__(name)
        
    def handleData(self, data):
        temp,batt = unpack("hh", data[0:4])
        checkBattery(self.nodeId, batt)
        if validateTemp(temp) != True:
            temp = None
        else:
	    temp = float(temp)/100.0
        return {"Node":self.name,"Temp":temp, "Battery":batt}
        
    
class TempSwitchSensor(TempSensor):
    def __init__(self, name, nodeId = None):
        super(TempSwitchSensor, self).__init__(name)
    def handleData(self, data):
        tempdata = super(TempSwitchSensor, self).handleData(data[0:4])
        tempdata['Switchstate'] = unpack("B", data[4])[0]
        return tempdata
