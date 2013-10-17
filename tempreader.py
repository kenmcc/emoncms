import struct
import urllib
import os
import datetime

fd = os.open("/dev/rfm12b.0.1",  os.O_NONBLOCK|os.O_RDWR)
localURI="http://192.168.1.31/emoncms/input/post.json?apikey=d959950e0385107e37e2457db27b781e&node="
remoteURI="http://emoncms.org/input/post.json?apikey=a6958b2d85dfdfab9406e1e786e38249&node="

run=True
while run == True:
  try:
	data=None
	jsonStr= ""
	print "Waiting for data"
	data = os.read(fd, 66)
	node, len = struct.unpack("BB", data[:2])
	now=datetime.datetime.now().strftime("%d.%m.%Y %H:%M")
	print now, node, len
	if node >= 10 and node <= 20:
		temp,batt = struct.unpack("hh", data[2:])
		if temp > -2000 and temp < 4000:
		  print "Got temp, batt = ", temp, batt
      	          jsonStr = "temp:"+str(float(temp/100.0))+",batt:"+str(float(batt/1000.0))
	elif node == 3:
	  rain,batt = struct.unpack("hh", data[2:])
	  print "Got rain, batt = ", temp, batt
	  jsonStr = "rain:"+str(float(rain/100.0))+",batt:"+str(float(batt/1000.0))

	
	if jsonStr != "":
	  for uri in [localURI, remoteURI]:
		url = uri+str(node)+"&json={"+jsonStr+"}"
	  	u = urllib.urlopen(url)
	        u.read()
  except:
    	print "Closing file"
	os.close(fd)
