/*
 Example for different sending methods
 
http://code.google.com/p/rc-switch/
 
 Edit by jfrmilner for Energenie PowerStrip using Serial input
*/
 
#include <RCSwitch.h>
 
RCSwitch mySwitch = RCSwitch();
byte inByte = 0;
void setup() {
 
 Serial.begin(9600);
 
 // Transmitter is connected to Arduino Pin #10
 mySwitch.enableTransmit(10);
 
// Optional set pulse length.
 // mySwitch.setPulseLength(320);
 
 // Optional set protocol (default is 1, will work for most outlets)
 // mySwitch.setProtocol(2);
 
 // Optional set number of transmission repetitions.
 // mySwitch.setRepeatTransmit(15);
 
 Serial.println("Ready"); // Ready to receive commands
}
 
void loop() {
 /* Switch using decimal code */
 if(Serial.available() > 0) { // A byte is ready to receive
 inByte = Serial.read();
 if(inByte == '1') { // byte is '1'
 mySwitch.send(4314015, 24);
 Serial.println("1 ON");
 }
 else if(inByte == '2') { // byte is '2'
 mySwitch.send(4314014, 24);
 Serial.println("1 OFF");
 }
 else if(inByte == '3') { // byte is '3'
 mySwitch.send(1298439, 24);
 Serial.println("2 ON");
 }
 else if(inByte == '4') { // byte is '4'
 mySwitch.send(1298438, 24);
 Serial.println("2 OFF");
 }
 else if(inByte == '5') { // byte is '5'
 mySwitch.send(4314011, 24);
 Serial.println("3 ON");
 }
 else if(inByte == '6') { // byte is '6'
 mySwitch.send(4314010, 24);
 Serial.println("3 OFF");
 }
 else if(inByte == '7') { // byte is '7'
 mySwitch.send(4314003, 24);
 Serial.println("4 ON");
 }
 else if(inByte == '8') { // byte is '8'
 mySwitch.send(4314002, 24);
 Serial.println("4 OFF");
 }
 else if(inByte == '9') { // byte is '9'
 mySwitch.send(4314013, 24);
 Serial.println("ALL ON");
 }
 else if(inByte == '0') { // byte is '0'
 mySwitch.send(4314012, 24);
 Serial.println("ALL OFF");
 }
 else { // byte isn't known
 Serial.println("Unknown");
 }
 }
}
