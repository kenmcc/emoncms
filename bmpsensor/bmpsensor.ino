//----------------------------------------------------------------------------------------------------------------------
// TinyTX - An ATtiny84 and BMP085 Wireless Air Pressure/Temperature Sensor Node
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Using the BMP085 sensor connected via I2C
// I2C can be connected withf SDA to D8 and SCL to D7 or SDA to D10 and SCL to D9
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
//----------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include <PortsBMP085.h> // Part of JeeLib
#include "DHT.h"


ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 2        // RF12 node ID in the range 1-30
#define network 99       // RF12 Network group
#define freq RF12_433MHZ  // Frequency of RFM12B module

//#define USE_ACK           // Enable ACKs, comment out to disable
#define RETRY_PERIOD 5    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 10       // Number of milliseconds to wait for an ack

//PortI2C i2c (2);         // BMP085 SDA to D8 and SCL to D7
PortI2C i2c (1);      // BMP085 SDA to D10 and SCL to D9
BMP085 psensor (i2c, 3); // ultra high resolution
#define BMP085_POWER 8   // BMP085 Power pin is connected on D9

#define DHTPOWER 7
#define DHTDATA 3
#define DHTTYPE DHT11

#define LED_PIN 0

DHT dht(DHTDATA, DHTTYPE);

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
    	  long int pres;	// Pressure reading
          long int humidity;
 } Payload;

 Payload tinytx;

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//-------------------------------------------------------------------------------------------------
 static void rfwrite(){
   #if defined(__AVR_ATtiny84__) 
     rf12_sleep(-1);              // Wake up RF module
     while (!rf12_canSend())
     rf12_recvDone();
     rf12_sendStart(0, &tinytx, sizeof tinytx); 
     rf12_sendWait(2);           // Wait for RF to finish sending while in standby mode
     rf12_sleep(0);              // Put RF module to sleep
#else
  Serial.print("Pretend Send:\n temp, voltage, press, hum\n");
  Serial.print(tinytx.temp);
  Serial.print(" ");
  Serial.print(tinytx.supplyV);	// Supply voltage
  Serial.print(" ");
  Serial.print(tinytx.pres);	// Pressure reading
  Serial.print(" ");
  Serial.print(tinytx.humidity);
  Serial.print("\n");

#endif
return;
 }

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------
long readVcc() {
   bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); // Enable the ADC
   long result;
   // Read 1.1V reference against Vcc
   #if defined(__AVR_ATtiny84__) 
    ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
   #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // For ATmega328
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
//########################################################################################################################

void setup() {
  Serial.begin(9600);
  pinMode(BMP085_POWER, OUTPUT); // set power pin for BMP085 to output
  pinMode(DHTPOWER, OUTPUT); // set power pin for BMP085 to output
  pinMode(LED_PIN, OUTPUT); // set power pin for BMP085 to output
  
  digitalWrite(BMP085_POWER, HIGH); // turn BMP085 sensor on

  Serial.write("Hello world\n");  
  Sleepy::loseSomeTime(50);
  psensor.getCalibData();

  digitalWrite(BMP085_POWER, LOW); // turn BMP085 sensor on
  digitalWrite(DHTPOWER, LOW);
  digitalWrite(LED_PIN, LOW); 
  
  #if defined(__AVR_ATtiny84__) 
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep
  #endif 
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
}
int lastPressureRead = 0;
int lastTempRead = 0;
float lastHumidityRead = 0;

#if defined(__AVR_ATtiny84__) 
#define LOOPTIME 60000
#else
#define LOOPTIME 6000
#endif

void loop() {

  delay(500);
  digitalWrite(LED_PIN, HIGH);
  int sleepTimer = millis() + LOOPTIME; 
  boolean again = false;
  digitalWrite(BMP085_POWER, HIGH); // turn BMP085 sensor on
  digitalWrite(DHTPOWER, HIGH);
  Sleepy::loseSomeTime(3000);

  dht.begin();
#if defined(__AVR_ATtiny84__) 
  do
  {
    again = false;
    // Get raw temperature reading
    psensor.startMeas(BMP085::TEMP);
    Sleepy::loseSomeTime(16);
    int32_t traw = psensor.getResult(BMP085::TEMP);
  
    // Get raw pressure reading
    psensor.startMeas(BMP085::PRES);
    Sleepy::loseSomeTime(32);
    int32_t praw = psensor.getResult(BMP085::PRES);
  
    // Calculate actual temperature and pressure
    int32_t press;
    int16_t thetemp;
    psensor.calculate(thetemp, press);
    tinytx.pres = (press * 0.01);
    tinytx.temp = thetemp * 100;
  
    if(tinytx.pres < lastPressureRead-1)
    {
      again = true;
    }
    else if (tinytx.pres > lastPressureRead + 1)
    {
      again = true;
    }
    else if(tinytx.temp < lastTempRead -1)
    {
      again = true;
    }
    else if(tinytx.temp > lastTempRead +1)
    {
      again = true;
    }
    lastTempRead = tinytx.temp;
    lastPressureRead = tinytx.pres;
    
  }while(again);
#endif

  float f;
  do
  {
    again = false;
    f = dht.readHumidity();
    if (f == NAN)
    {
      again = true;
    }
    else if (f < lastHumidityRead - 1)
    {
      again = true;
    }
    else if (f > lastHumidityRead + 1)
    {
      again = true;
    }
    else
    {
      tinytx.humidity = (int)(f * 100);
    }
    if (f != NAN)
    {
      lastHumidityRead = f;
    }
  }while(again);
  tinytx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF 

  digitalWrite(BMP085_POWER, LOW); // turn BMP085 sensor off
  digitalWrite(DHTPOWER, LOW);     // turn DHT11 off
  digitalWrite(LED_PIN, LOW);
  
  Sleepy::loseSomeTime(sleepTimer - millis()); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
}

