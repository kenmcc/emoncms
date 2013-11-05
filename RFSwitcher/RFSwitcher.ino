#include <RCSwitch.h>
#include <PinChangeInterrupt.h>
#include <JeeLib.h> // https://github.com/jcw/jeelib
#include <avr/sleep.h>

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define TX_POWER 10
#define TX_DATA 9

#define LOW_TEMP_PIN 3
#define HIGH_TEMP_PIN 7

RCSwitch mySwitch = RCSwitch();
void setup() {
  
   Serial.begin(9600);
   pinMode(TX_POWER, OUTPUT);
   digitalWrite(TX_POWER, LOW);

   // Transmitter is connected to Arduino Pin #10
   mySwitch.enableTransmit(TX_DATA);

   PRR = bit(PRTIM1); // only keep timer 0 going
  
   ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
 
   Serial.println("Ready"); // Ready to receive commands
   
  pinMode(LOW_TEMP_PIN, INPUT);     // set reed pin as input
  pinMode(HIGH_TEMP_PIN, INPUT);
  
  //digitalWrite(reedPin, HIGH); // and turn on pullup
  attachPcInterrupt(LOW_TEMP_PIN,wakeUpLow,RISING); // attach a PinChange Interrupt on the falling edge
  attachPcInterrupt(HIGH_TEMP_PIN,wakeUpHigh,RISING);
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);         // Set sleep mode
  sleep_mode();                                // Sleep now
   
}

volatile boolean tempTooHigh = false;
volatile boolean tempTooLow = false;
void wakeUpLow()
{
  tempTooLow = true;
}
void wakeUpHigh()
{
  tempTooHigh = true;
}

volatile int never = 0;
void loop() {
  
  if(never)
  {
    Serial.available();
  }
 /* Switch using decimal code */
  if (tempTooLow)
  {
    mySwitch.send(1298439, 24);
  }
  else
  {
    mySwitch.send(1298438, 24);
  }
 
  

}
