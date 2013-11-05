#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <JeeLib.h> // https://github.com/jcw/jeelib

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define ONE_WIRE_BUS 10   // DS18B20 Temperature sensor is connected on D10/ATtiny pin 13
#define ONE_WIRE_POWER 9  // DS18B20 Power pin is connected on D9/ATtiny pin 12

#define RED_LED 0
#define BLUE_LED 3
#define GREEN_LED 7

#define HIGH_TEMP 18
#define LOW_TEMP 16

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance

DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature
 
void setup() {
   Serial.begin(9600);
   pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
   pinMode(RED_LED, OUTPUT);
   pinMode(BLUE_LED, OUTPUT);
   pinMode(GREEN_LED, OUTPUT);
   digitalWrite(ONE_WIRE_POWER, LOW);
   digitalWrite(RED_LED, LOW);
   digitalWrite(BLUE_LED, LOW);
   digitalWrite(GREEN_LED, LOW);

   PRR = bit(PRTIM1); // only keep timer 0 going
  
   ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
 
   Serial.println("Ready"); // Ready to receive commands
}
 

void doLed(int led, int valOne, int dTime, int valTwo)
{
  digitalWrite(led, valOne);
  delay(dTime);
  digitalWrite(led, valTwo);
}

long readVcc() {
   bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); // Enable the ADC
   long result;
   // Read 1.1V reference against Vcc
   #if defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
   #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); // For ATmega328
   #endif
   delay(2); // Wait for Vref to settle
   ADCSRA |= _BV(ADSC); // Convert
   while (bit_is_set(ADCSRA,ADSC));
   result = ADCL;
   result |= ADCH<<8;
   result = 1126400L / result; // Back-calculate Vcc in mV
   ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
   return result;
} 

int lastReadTemp = 0;

void loop() {
 
  int temp = 0;
  int readAgain = 1;
  digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on

  Sleepy::loseSomeTime(5); // Allow 5ms for the sensor to be ready
  delay(5); // The above doesn't seem to work for everyone (why?)
 
  sensors.begin(); //start up temp sensor
  
  while(readAgain)
  {
    sensors.requestTemperatures(); // Get the temperature
    delay(10);
    temp = sensors.getTempCByIndex(0);
  
    if (temp > lastReadTemp + 1 || temp < lastReadTemp -1)
    {
      lastReadTemp = temp;
    }
    else 
    {
      readAgain = 0;
    }
  }

  if(temp >18)
  {
      Serial.println("Too HIGH!");
      doLed(RED_LED, HIGH, 250, LOW);
  }
  else if (temp < 16)
  {
    Serial.println("Too LOW!");
    doLed(BLUE_LED, HIGH, 250, LOW);
  }
  else
  {
    Serial.println("Just Right!");
    doLed(GREEN_LED, HIGH, 250, LOW);
  }
  
  if(readVcc() < 2750)
  {
    Serial.println("Low Battery!");
    for(int i = 0; i < 10; i++)
    {
      doLed(GREEN_LED, HIGH, 10, HIGH);
      delay(50);
      doLed(GREEN_LED, LOW, 10, LOW);
      delay(50);
    }
  }

  Sleepy::loseSomeTime(60000);

}

