import struct
import urllib
import os
import datetime
import sys
import subprocess
from emailer import *

fd = os.open("/dev/rfm12b.0.1",  os.O_NONBLOCK|os.O_RDWR)
localURI="http://192.168.1.31/emoncms/input/post.json?apikey=d959950e0385107e37e2457db27b781e&node="
remoteURI="http://emoncms.org/input/post.json?apikey=a6958b2d85dfdfab9406e1e786e38249&node="
#goatstownURI="http://goatstownweather.hostoi.com/emoncms/input/post.json?apikey=0f170b829035f4cde06637f953852333&node="

timeDelta = datetime.timedelta(minutes=5)


latestData = {
"time":(datetime.datetime.now()-timeDelta).strftime("%Y-%m-%d %H:%M:%S"), 
"delay":1, 
"hum_in":50, 
"temp_in":20, 
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


def handleTempSensor(node, data=None):
  temp,batt = struct.unpack("hh", data)
  checkBattery(node, batt)
  if validateTemp(temp):
    print "Got temp, batt = ", temp, batt
    jsonStr = "temp:"+str(float(temp/100.0))+",batt:"+str(float(batt/1000.0))
    return jsonStr,float(temp/100.0)
  else:
    print "Ignoring invalid Temp: ", temp
  return "",0.0

def handlePressureSensor(node, data):
  temp, batt, pressure = struct.unpack("hhi", data)
  checkBattery(node, batt)
  if validateTemp(temp) and validatePressure(pressure):
    print "Got temp, battery, pressure = ", temp, batt, pressure
    jsonStr = "temp:"+str(float(temp/10.0))
    jsonStr += ",batt:"+str(float(batt/1000.0))
    jsonStr += ",pressure:"+str(pressure)  
    return jsonStr,pressure,float(temp/10.0)
  else:
    print "Ignoring invalid temp or pressure", temp, pressure
  return "", 0, 0.0

def handleRainGauge(node, data):
  print "rain sensor"
  rain,batt = struct.unpack("hh", data)
  checkBattery(node, batt)
  jsonStr = "rain:"+str(float(rain/100.0))+",batt:"+str(float(batt/1000.0))
  return jsonStr,rain





def populateCurrentData():
  Y,M,D=getYearMonthDay()
  YM="-".join([str(Y),str(M)])
  YMD="-".join([YM,str(D)])
  path = subprocess.check_output(["find "+fileBaseDir+" -name \"*.txt\" | sort | tail -n 1" ], shell=True)
  print "Found this as the last entry" , path
  if os.path.exists(path):
    lastEntry= subprocess.check_output(['tail', '-1', path])
    if lastEntry is not None and len(lastEntry) > 10:
      fields=lastEntry.split(",")
      if len(fields) == 11:
        latestData["time"] = fields[0]
        latestData["delay"] = fields[1] 
        latestData["hum_in"] = fields[2] 
        latestData["temp_in"] = fields[3] 
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
    dataToWrite = ",".join([str(latestData["time"]),str(latestData["delay"]),str(latestData["hum_in"]),str(latestData["temp_in"]),str(latestData["hum_out"]),str(latestData["temp_out"]),str(latestData["pressure"]),str(latestData["wind_avg"]),str(latestData["wind_gust"]),str(latestData["wind_dir"]),str(latestData["rain"])] )
    print "would write", dataToWrite
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
      nowtime = datetime.datetime.now()
      now= nowtime.strftime("%Y-%m-%d %H:%M:%S")
      print now
      #print now, node, len
      if node == 3 and datalen == 4:
        jsonStr,rain = handleRainGauge(node, data[2:])
        latestData["rain"]=rain
        
      elif node >= 10 and node < 20 and datalen == 4:
        jsonStr,temp = handleTempSensor(node, data[2:])
        latestData["temp_in"] = temp
        
      elif node == 20 and datalen == 8: # this is a pressure sensor 
        jsonStr,pressure,outtemp = handlePressureSensor(node, data[2:])
        latestData["pressure"]=pressure
        latestData["temp_out"]=outtemp
      else:
        print "don't know what to do with node, len", node, datalen    

      if jsonStr != "":
        for uri in [remoteURI]:
          url=""
          try:
            url = uri+str(node)+"&json={"+jsonStr+"}"
            u = urllib.urlopen(url)
            #u.read()
          except:
            print "Failed to connect to ", url

      print "seeing if i need to do some writing"      
      lastruntime = datetime.datetime.strptime(latestData["time"], "%Y-%m-%d %H:%M:%S")
      if nowtime >= lastruntime + timeDelta:
        print "time to update!"
        latestData["time"] = now
        writeDataToFile()
      else:
        print "Not time yet", nowtime, lastruntime + timeDelta
      
    #except:
    #print "Closing file"
    #os.close(fd)
    #run = False
