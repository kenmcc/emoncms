import struct
import urllib
import os
import datetime
import sys
import subprocess
from emailer import *
from Sensor import *
import logging

logging.basicConfig(filename='tempreaderpy.log',level=logging.DEBUG,format='%(asctime)s %(message)s')

fd = os.open("/dev/rfm12b.0.1",  os.O_NONBLOCK|os.O_RDWR)

timeDelta = datetime.timedelta(minutes=1)

SENSORS = { 1:["COLLECTOR",None], 
            2:["WS:PTHum",PressureHumiditySensor],
            #3:["WS:WindRain",RainSensor],
            10:["Kitchen", TempSensor],
            #11:["SittingRoom", TempSensor],
            21:["SadhbhBedroom",TempSwitchSensor], 
}


latestData = {
"time":(datetime.datetime.now()-timeDelta).strftime("%Y-%m-%d %H:%M:%S"), 
"delay":1, 
"hum_in":50, 
"temp_in_avg":20, 
"temp_in_sensors":{},
"hum_out":50, 
"temp_out":15, 
"pressure":1000, 
"wind_avg":0, 
"wind_gust":0, 
"wind_dir":0, 
"rain":0}


fileBaseDir = "/home/pi/weatherlogger-db/data/raw/"

notifiedLowBatteries = []

def getYearMonthDay():
  Y=datetime.datetime.now().year
  M=datetime.datetime.now().month
  D=datetime.datetime.now().day
  return Y,M,D


def populateCurrentData():
  Y,M,D=getYearMonthDay()
  YM="-".join([str(Y),str(M)])
  YMD="-".join([YM,str(D)])
  path = subprocess.check_output(["find "+fileBaseDir+" -name \"*.txt\" | sort | tail -n 1" ], shell=True)
  logging.info("Found this as the last entry" + path)
  if os.path.exists(path):
    lastEntry= subprocess.check_output(['tail', '-1', path])
    if lastEntry is not None and len(lastEntry) > 10:
      fields=lastEntry.split(",")
      if len(fields) == 11:
        latestData["time"] = fields[0]
        latestData["delay"] = fields[1] 
        latestData["hum_in"] = fields[2] 
        latestData["temp_in_avg"] = fields[3] 
        latestData["hum_out"] = fields[4] 
        latestData["temp_out"] = fields[5] 
        latestData["pressure"] = fields[6] 
        latestData["wind_avg"] = fields[7] 
        latestData["wind_gust"] = fields[8] 
        latestData["wind_dir"] = fields[9] 
        latestData["rain"] = fields[10].strip().replace("\n", "")

def writeDataToFile():
  Y,M,D=getYearMonthDay()
  YM="-".join([str(Y),str(M)])
  YMD="-".join([YM,str(D)])
  path=fileBaseDir+"/"+str(Y)+"/"+YM+"/"+YMD+".txt"

  with open(path, "a") as f:
    dataToWrite = ",".join([str(latestData["time"]),str(latestData["delay"]),str(latestData["hum_in"]),str(latestData["temp_in_avg"]),str(int(latestData["hum_out"])),str(latestData["temp_out"]),str(latestData["pressure"]),str(latestData["wind_avg"]),str(latestData["wind_gust"]),str(latestData["wind_dir"]),str(latestData["rain"])] )
    logging.info("writing %s", dataToWrite)
    f.write(dataToWrite+"\n")        

if __name__=="__main__":
  populateCurrentData()  
  run=True
  while run == True:
      #try:
      data=None
      jsonStr= ""
      data = os.read(fd, 66)
      if data is not None and len(data) > 2:
          node, datalen = struct.unpack("BB", data[:2])
          payload = data[2:]
          nowtime = datetime.datetime.now()
          now= nowtime.strftime("%Y-%m-%d %H:%M:%S")
          logging.info("Message from Node = %s ", node)
          if node in SENSORS:
              sensorName = SENSORS[node][0]
              sensorFunc = SENSORS[node][1]
              latestData = sensorFunc(sensorName, node).handleData(payload, latestData)

          else:
              logging.warning("don't know what to do with node %s, len %s", node, datalen)
          '''    
          #print now, node, len
          elif if node == 3 and datalen == 4:
              jsonStr,rain = handleRainGauge(node, data[2:])
              latestData["rain"]=rain
        
          elif node >= 10 and node < 20 and datalen == 4:
              jsonStr,temp = handleTempSensor(node, data[2:])
              latestData["temp_in_sensors"][node] = temp
        
          elif node == 2 and datalen == 12: # this is a pressure sensor 
              #jsonStr,pressure,outtemp = handlePressureSensor(node, data[2:])
              #latestData["pressure"]=pressure
              #latestData["temp_out"]=outtemp
              sensorName = SENSORS[node][0]
              sensorFunc = SENSORS[node][1]
              processed_data = sensorFunc(sensorName, node).handleData(payload)
              print processed_data
              if processed_data["Temp"] is not None:
                  latestData["temp_out"] = processed_data["Temp"]
      
          elif node == 21 and datalen == 5:
              sensorName = SENSORS[node][0]
              sensorFunc = SENSORS[node][1]
              processed_data = sensorFunc(sensorName, node).handleData(payload)
              if processed_data["Temp"] is not None:
                  latestData["temp_in_sensors"][node] = processed_data["Temp"]
              print processed_data
          '''
          
              

      lastruntime = datetime.datetime.strptime(latestData["time"], "%Y-%m-%d %H:%M:%S")
      if nowtime >= lastruntime + timeDelta:
        logging.info("time to update!")
        latestData["time"] = now
        if len(latestData["temp_in_sensors"]) > 0:
            latestData["temp_in_avg"] = float(sum(latestData["temp_in_sensors"].values()))/len(latestData["temp_in_sensors"])  
        writeDataToFile()
      
    #except:
    #print "Closing file"
    #os.close(fd)
    #run = False
