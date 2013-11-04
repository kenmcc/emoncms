#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <JeeLib.h> // https://github.com/jcw/jeelib

#define RCSwitchDisableReceiving

#include <RCSwitch.h>

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define ONE_WIRE_BUS 10   // DS18B20 Temperature sensor is connected on D10/ATtiny pin 13
#define ONE_WIRE_POWER 9  // DS18B20 Power pin is connected on D9/ATtiny pin 12

#define TX_POWER 8
#define TX_DATA 7

#define RED_LED 0
#define BLUE_LED 3

#define HIGH_TEMP 18
#define LOW_TEMP 16

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance

DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature
 
RCSwitch mySwitch = RCSwitch();
void setup() {
   Serial.begin(9600);
   pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
   pinMode(TX_POWER, OUTPUT);
   pinMode(RED_LED, OUTPUT);
   pinMode(BLUE_LED, OUTPUT);
   digitalWrite(ONE_WIRE_POWER, LOW);
   digitalWrite(TX_POWER, LOW);
   digitalWrite(RED_LED, LOW);
   digitalWrite(BLUE_LED, LOW);
   

   // Transmitter is connected to Arduino Pin #10
   mySwitch.enableTransmit(TX_DATA);

   PRR = bit(PRTIM1); // only keep timer 0 going
  
   ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
 
   Serial.println("Ready"); // Ready to receive commands
}
 
void mydoSend(int val)
{
  if (val == 1)
  {
    mySwitch.send(1298439, 24);
  }
  else
  {
    mySwitch.send(1298438, 24);
  }
}

void redLed(int val)
{
  digitalWrite(RED_LED, (val == 1? HIGH: LOW));
}
void blueLed(int val)
{
  digitalWrite(BLUE_LED, (val == 1? HIGH: LOW));
}


volatile int never = 0;
void loop() {
 /* Switch using decimal code */
 
   if (never)
   {
     Serial.available(); // for some reason, this seems to be needed to make this compile 
   }

  int tempX = 20;
  int temp = 0;
  digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on

  //Sleepy::loseSomeTime(5); // Allow 5ms for the sensor to be ready
  delay(5); // The above doesn't seem to work for everyone (why?)
 
 // sensors.begin(); //start up temp sensor
 // sensors.requestTemperatures(); // Get the temperature
 //   tempX = sensors.getTempCByIndex(0);

  if(tempX >18)
  {
//    redLed(1);
    mydoSend(0);
  }
  else if (tempX < 16)
  {
//    blueLed(1);
  //  mydoSend(1);
      //mySwitch.send(1298439, 24);

  }
  else
  {
//    blueLed(1);
//    redLed(1);
  }

}
