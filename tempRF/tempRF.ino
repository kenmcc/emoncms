#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <JeeLib.h> // https://github.com/jcw/jeelib
#include <RCSwitch.h>
#include <avr/sleep.h>

ISR(WDT_vect) { 
  Sleepy::watchdogEvent(); 
} // interrupt handler for JeeLabs Sleepy power saving

#define ONE_WIRE_BUS 7   // DS18B20 Temperature sensor is connected on D10/ATtiny pin 13
#define ONE_WIRE_POWER 8   // DS18B20 Power pin is connected on D9/ATtiny pin 12

#define TX_POWER 10    // the Tx Power is on pin 8
#define TX_DATA 9     // and the data is on pin 7

#define LED 3         // le pin 3

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
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature
RCSwitch mySwitch = RCSwitch();


void setup() {
  Serial.begin(9600);
  
  pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
  pinMode(TX_POWER, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(TX_POWER, LOW);
  digitalWrite(ONE_WIRE_POWER, LOW);
  digitalWrite(LED, LOW);

  mySwitch.enableTransmit(TX_DATA);

  PRR = bit(PRTIM1); // only keep timer 0 going

  ADCSRA &= ~ bit(ADEN); 
  bitSet(PRR, PRADC); // Disable the ADC to save power

  Serial.println("Ready"); // Ready to receive commands

//  set_sleep_mode(SLEEP_MODE_PWR_DOWN);         // Set sleep mode
//  sleep_mode();                                // Sleep now
}


void doLed(int led, int valOne, int dTime, int valTwo, int dTime2, int count)
{
  for(count; count > 0; count--)
  {
    digitalWrite(led, valOne);
    delay(dTime);
    digitalWrite(led, valTwo);
    delay(dTime2);
  }
}

long readVcc() {
  bitClear(PRR, PRADC); 
  ADCSRA |= bit(ADEN); // Enable the ADC
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
  ADCSRA &= ~ bit(ADEN); 
  bitSet(PRR, PRADC); // Disable the ADC to save power
  return result;
} 

int lastReadTemp = 0;

int getTemp()
{
  int temp = 0;
  boolean readAgain = true;
  digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on

  //Sleepy::loseSomeTime(5); // Allow 5ms for the sensor to be ready
  delay(25); // The above doesn't seem to work for everyone (why?)

  sensors.begin(); //start up temp sensor
  while(readAgain)
  {
    sensors.requestTemperatures(); // Get the temperature
    delay(5);
    temp = sensors.getTempCByIndex(0);

    if (temp > lastReadTemp + 1 || temp < lastReadTemp -1)
    {
      lastReadTemp = temp;
    }
    else 
    {
      readAgain = false;
    }
  }
  digitalWrite(ONE_WIRE_POWER, LOW); // turn DS18B20 sensor on
  return temp;
}

void sendRF(long codeToSend)
{
  digitalWrite(TX_POWER, HIGH);
  Sleepy::loseSomeTime(100);
  Serial.print("Sending Code ");
  Serial.print(codeToSend);
  Serial.print("\n");
  mySwitch.send(codeToSend, 24);
  digitalWrite(TX_POWER, LOW);
}

volatile int loopCounter = -1;
void loop() {
  loopCounter++;
  if (loopCounter%LOOP_COUNTER == 0)
  {
    int temp = getTemp();

    if(temp >HIGH_TEMP)
    {
      Serial.write("Too HIGH!\n");
      Serial.println(temp);
      delay(2);
      doLed(LED, HIGH, 100, LOW, 100, 5);
      sendRF(TURN_OFF_CODE);
    }
    else if (temp < LOW_TEMP)
    {
      Serial.write("Too LOW!\n");
      Serial.println(temp);
      delay(2);
      doLed(LED, HIGH, 400, LOW, 400, 3);
      sendRF(TURN_ON_CODE);
    }
    else
    {
      Serial.println("Just Right!");
      Serial.println(temp);
      doLed(LED, HIGH, 100, LOW,100,1);
    }

    /* let the thing settle a wee bit */
    Sleepy::loseSomeTime(2000);
    /* check the battery and do a battery signal */
    long battery = readVcc();
    if(battery < 2750)
    {
      Serial.println("Low Battery!");
      Serial.println(battery);
      doLed(LED, HIGH, 100,LOW,100,3); /* S O S */
      // ...
      doLed(LED, HIGH, 300,LOW,300,3);             // ---
      doLed(LED, HIGH, 100,LOW,100,3);             // ...
    }
    loopCounter = 0;
  }

  /* and then head off to sleep */
  Sleepy::loseSomeTime(TIME_BETWEEN_READINGS);

}


