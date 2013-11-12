#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <JeeLib.h> // https://github.com/jcw/jeelib
#include <RCSwitch.h>
#include <avr/sleep.h>

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define TX_POWER 10    // the Tx Power is on pin 8
#define TX_DATA 9     // and the data is on pin 7
#define ONE_WIRE_POWER 8   // DS18B20 Power pin is connected on D9/ATtiny pin 12
#define ONE_WIRE_BUS 7   // DS18B20 Temperature sensor is connected on D10/ATtiny pin 13
#define LED 3         // LED  is on pin 3

#define TURN_ON_CODE 1298439
#define TURN_OFF_CODE 1298438 

#if defined(__AVR_ATtiny84__)
  #define TIME_BETWEEN_READINGS 60000
  #define LOOP_COUNTER 5
  #define HIGH_TEMP 18
  #define LOW_TEMP 16
#else
  #define TIME_BETWEEN_READINGS 6000
  #define LOOP_COUNTER 1
  #define HIGH_TEMP 24
  #define LOW_TEMP 22
#endif

#define myNodeID 21        // RF12 node ID in the range 1-30
#define network 99       // RF12 Network group
#define freq RF12_433MHZ  // Frequency of RFM12B module

#define SWITCH_NO_CHANGE 2
#define SWITCH_ON 1
#define SWITCH_OFF 0

typedef struct {
  	  int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
          byte switchValue; 
 } Payload;
 Payload tinytx;


OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature
RCSwitch mySwitch = RCSwitch();

#if defined(__AVR_ATtiny84__)
static void rfwrite(){
     rf12_sleep(-1);              // Wake up RF module
     while (!rf12_canSend())
     rf12_recvDone();
     rf12_sendStart(0, &tinytx, sizeof tinytx); 
     rf12_sendWait(2);           // Wait for RF to finish sending while in standby mode
     rf12_sleep(0);              // Put RF module to sleep
     return;
 }
#else
static void rfwrite(){
  Serial.println("DUMMY SEND : \nTEMP\t Voltage\t Switch ");
  Serial.print(tinytx.temp);
  Serial.print("\t");
  Serial.print(tinytx.supplyV);
  Serial.print("\t");
  Serial.print((int)tinytx.switchValue);
  Serial.print("\t");
  return;
}
#endif

int readVcc() {
  bitClear(PRR, PRADC); 
  ADCSRA |= bit(ADEN); // Enable the ADC
  int  result;
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
  ADCSRA &= ~ bit(ADEN); 
  bitSet(PRR, PRADC); // Disable the ADC to save power
  return result;
} 

int lastReadTemp = 0;
static int getTemp()
{
  int temp = 0;
  boolean readAgain = true;
  digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on

  Sleepy::loseSomeTime(10); // The above doesn't seem to work for everyone (why?)

  sensors.begin(); //start up temp sensor
  do
  {
    sensors.requestTemperatures(); // Get the temperature
    Sleepy::loseSomeTime(5);
    temp = (int)(sensors.getTempCByIndex(0)*100);
    /* sanity check the temperature. make sure it's not wildly different than last time */
    if (temp < lastReadTemp + 1 && temp > lastReadTemp -1)
    {
      readAgain = false;
    }
    lastReadTemp = temp;
  }while(readAgain);
  digitalWrite(ONE_WIRE_POWER, LOW); // turn DS18B20 sensor off
  return temp;
}

static void sendRF(byte switchAction)//long codeToSend)
{
  digitalWrite(TX_POWER, HIGH);
  Sleepy::loseSomeTime(100);
  mySwitch.send((switchAction==SWITCH_ON)?TURN_ON_CODE:TURN_OFF_CODE, 24);
  digitalWrite(TX_POWER, LOW);
}

static void ledOn(int ontime) // milliseconds
{
  digitalWrite(LED, HIGH);
  Sleepy::loseSomeTime(ontime);
  digitalWrite(LED, LOW);
}

void setup() {
  //Serial.begin(9600);
#if defined(__AVR_ATtiny84__)
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  //rf12_sleep(0);                          // Put the RFM12 to sleep
#else
  Serial.begin(9600);
#endif
  
  pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
  pinMode(TX_POWER, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(TX_POWER, LOW);
  digitalWrite(ONE_WIRE_POWER, LOW);
  digitalWrite(LED, LOW);
  
  ledOn(1000);

  mySwitch.enableTransmit(TX_DATA);

  PRR = bit(PRTIM1); // only keep timer 0 going

  ADCSRA &= ~ bit(ADEN); 
  bitSet(PRR, PRADC); // Disable the ADC to save power

}

//volatile int loopCounter = -1;
void loop() {
//  loopCounter++;
  //if (loopCounter%LOOP_COUNTER == 0)
  {
    tinytx.temp = getTemp();
    if(tinytx.temp > HIGH_TEMP*100)
    {
   //   ledOn(200);
      sendRF(SWITCH_OFF);
      tinytx.switchValue = SWITCH_OFF;
    }
    else if (tinytx.temp < LOW_TEMP*100)
    {
     // ledOn(500);
      sendRF(SWITCH_ON);
      tinytx.switchValue = SWITCH_ON;    
    }
    else
    {
      tinytx.switchValue = SWITCH_NO_CHANGE;
      //ledOn(50);
    }

    /* let the thing settle a wee bit */
    Sleepy::loseSomeTime(200);
    /* check the battery and do a battery signal */
    tinytx.supplyV = readVcc();
    
    rfwrite();
 //   loopCounter = 0;
    
  }

  /* head off to sleep */
  Sleepy::loseSomeTime(TIME_BETWEEN_READINGS);
}


