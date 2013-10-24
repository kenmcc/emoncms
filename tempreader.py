import struct
import urllib
import os
import datetime

fd = os.open("/dev/rfm12b.0.1",  os.O_NONBLOCK|os.O_RDWR)
localURI="http://192.168.1.31/emoncms/input/post.json?apikey=d959950e0385107e37e2457db27b781e&node="
remoteURI="http://emoncms.org/input/post.json?apikey=a6958b2d85dfdfab9406e1e786e38249&node="
#goatstownURI="http://goatstownweather.hostoi.com/emoncms/input/post.json?apikey=0f170b829035f4cde06637f953852333&node="


run=True
while run == True:
  	#try:
	data=None
	jsonStr= ""
	data = os.read(fd, 66)
	if data is not None and len(data) > 2:
  	  node, datalen = struct.unpack("BB", data[:2])
	  now=datetime.datetime.now().strftime("%d.%m.%Y %H:%M")
	  #print now, node, len
	  if node == 3 and datalen == 4:
            print "rain sensor"
	    rain,batt = struct.unpack("hh", data[2:])
	    jsonStr = "rain:"+str(float(rain/100.0))+",batt:"+str(float(batt/1000.0))
	  elif node >= 10 and node < 20 and datalen == 4:
	    temp,batt = struct.unpack("hh", data[2:])
	    if temp > -2000 and temp < 4000:
	      print "Got temp, batt = ", temp, batt
   	      jsonStr = "temp:"+str(float(temp/100.0))+",batt:"+str(float(batt/1000.0))
	  elif node == 20 and datalen == 8: # this is a pressure sensor 
	    temp, batt, pressure = struct.unpack("hhi", data[2:])
	    if temp >-2000 and temp < 40000 and pressure >900 and pressure < 1100:
	 	print "Got temp, battery, pressure = ", temp, batt, pressure
		jsonStr = "temp:"+str(float(temp/10.0))
	  	jsonStr += ",batt:"+str(float(batt/1000.0))
          	jsonStr += ",pressure:"+str(pressure)  
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
  	#except:
    	#print "Closing file"
os.close(fd)
