from struct import *

def checkBattery(node, battV):
  if battV < 2750 and node not in notifiedLowBatteries:
    sendEmail("Low Battery Warning", "Battery on node " + node + " is low: " + battV, "ken.mccullagh@gmail.com")
    notifiedLowBatteries.append(node)

def validateTemp(temp):
  if temp > -2000 and temp < 4000:
    return True
  return False
  
def validatePressure(pressure):
  if pressure > 900 and pressure < 1100:
    return True
  return False  
  

class Sensor(object):
    def __init__(self, name, nodeId = None):
        self.name = name
        self.nodeId = nodeId
       
    def handleData(self, payload, storedData = None):
        return None
        
    def printData(self, data):
        print "Node:{0};{1}".format(self.nodeId, self.name), data 

    
class UnKnown(Sensor):
    def handleData(self, payload, storedData = None):
        pass

class HumiditySensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(HumiditySensor, self).__init__(name, nodeId)
    
    def handleData(self, payload, storedData = None):
        humidity, batt = unpack("hh", payload[0:4])
        checkBattery(self.nodeId, batt)
        ##return {"Node":self.name,"Humidity":humidity, "Battery":batt}

class PressureHumiditySensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(PressureHumiditySensor, self).__init__(name, nodeId)
    
    def handleData(self, payload, storedData = None):
        #if len(payload) == 12:
            temp, batt, pressure, humidity = unpack("hhii", payload[0:12])
            self.printData({"Temp":temp, "Battery":batt, "Pressure": pressure, "Humidity":humidity})
            checkBattery(self.nodeId, batt)
            if validateTemp(temp) == True and validatePressure(pressure) == True:
                temp = float(temp)/10.0
                humidity = float(humidity)/100.0
                if storedData is not None:
                    storedData["pressure"] = pressure
                    storedData["hum_out"] = humidity
                    storedData["temp_out"] = temp
                    


class RainSensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(RainSensor, self).__init__(name, nodeId)
    
    def handleData(self, payload, storedData = None):
        rain,batt = unpack("hh", payload[0:4])
        checkBattery(self.nodeId, batt)
        print "Rain Sensor; don't store", rain, batt
        #return {"Node":self.name, "Rain":rain, "Battery":batt}    

class TempSensor(Sensor):
    def __init__(self, name, nodeId = None):
        super(TempSensor, self).__init__(name, nodeId)
        
    def handleData(self, payload, storedData = None):
        #if len(payload) == 4:
            temp,batt = unpack("hh", payload[0:4])
            self.printData({"Temp":temp, "Batt":batt})
            checkBattery(self.nodeId, batt)
            if validateTemp(temp) != True:
                temp = None
            else:
                temp = float(temp)/100.0
                if storedData is not None:
                    storedData["temp_in_sensors"][self.nodeId] = temp           
        

'''
    this is a special class of temperature sensor, which also can send an RF command
    to a power switch to turn on or off 
'''
class TempSwitchSensor(TempSensor):
    def __init__(self, name, nodeId = None):
        super(TempSwitchSensor, self).__init__(name, nodeId)
        
    def handleData(self, payload, storedData = None):
        #if len(payload) == 5:
            tempdata = super(TempSwitchSensor, self).handleData(payload[0:4], storedData)
            if storedData is not None:
                   storedData["Switchstate"] = unpack("B", payload[4])[0]
            
        
        
