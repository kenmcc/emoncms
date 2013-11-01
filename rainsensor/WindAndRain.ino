#include <JeeLib.h> // https://github.com/jcw/jeelib
#include <PinChangeInterrupt.h> // http://code.google.com/p/arduino-tiny/downloads/list
#include <avr/sleep.h>




#define ANEMOMETER_PIN 3
#define ANEMOMETER_INT 1
#define VANE_PWR 4
#define VANE_PIN A0
#define RAIN_GAUGE_PIN 2
#define RAIN_GAUGE_INT 0

#define myNodeID 3 // RF12 node ID in the range 1-30
#define network 99 // RF12 Network group
#define freq RF12_433MHZ // Frequency of RFM12B module

typedef struct {
           int rain;        // Rainfall
           int maxRainfallRate; // mm/hour
           int avgWindSpeed; // km/h
           int windGustMax; // km/h 
           byte windDir; // 0-15 clockwise from N
           int supplyV;        // Supply voltage
 } Payload;
Payload tinytx;

 
typedef struct RainData{
   double rainMM;
   int maxRainRate;
   int minRainRate;
}RainData;

typedef struct WindData{
   double avgSpeed;
}WindData;

static void rfwrite(){
  #if defined(__AVR_ATtiny84__)
     rf12_sleep(-1); // Wake up RF module
     while (!rf12_canSend())
     rf12_recvDone();
     rf12_sendStart(0, &tinytx, sizeof tinytx);
     rf12_sendWait(2); // Wait for RF to finish sending while in standby mode
     rf12_sleep(0); // Put RF module to sleep
     return;
  #else
     Serial.println("DUMMY SEND: ");
  #endif  
}
static void rfSetup()
{
  #if defined(__AVR_ATtiny84__)
    rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above
    rf12_sleep(0); // Put the RFM12 to sleep
  #else
    Serial.begin(9600);
  #endif  
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
 
void setupWeatherInts()
{
  pinMode(ANEMOMETER_PIN,INPUT);
  digitalWrite(ANEMOMETER_PIN,HIGH);  // Turn on the internal Pull Up Resistor
  pinMode(RAIN_GAUGE_PIN,INPUT);
  digitalWrite(RAIN_GAUGE_PIN,HIGH);  // Turn on the internal Pull Up Resistor
  pinMode(VANE_PWR,OUTPUT);
  digitalWrite(VANE_PWR,LOW);
  attachInterrupt(ANEMOMETER_INT,anemometerClick,FALLING);
  attachInterrupt(RAIN_GAUGE_INT,rainGageClick,FALLING);
  interrupts();
}

#define WIND_FACTOR 2.4 // km/h per rotation per sec
#define TEST_PAUSE 60000
 
volatile unsigned long anemRotations=0;
volatile unsigned long anem_last=0;
volatile unsigned long anem_min=0xffffffff;
volatile unsigned long anem_max=0;
 
struct WindData getAvgWindspeed(unsigned long period)
{
  struct WindData data;
  // avg windspeed is (rotations x distance per rotation)/time in hours
  // km/h = rev/min * 2.4
  unsigned long reading=anemRotations;
  anemRotations=0;
  // windspeed = num revs / revs/min * 2.4
  double revPerMin = reading / (period/1000/60);  
  data.avgSpeed = (reading/ revPerMin) * WIND_FACTOR;
  return data;
}
 
double getGust()
{
  unsigned long reading=anem_min;
  anem_min=0xffffffff;
  double time=reading/1000000.0;
  return (1/(reading/1000000.0))*WIND_FACTOR;
}
 
/* this is called by interrupt when the anemometer rotates. */ 
void anemometerClick()
{
  /* software debounce - 500ms between counts */
  unsigned long now = micros(); // timestamp now
  long thisTime= now - anem_last; /* time since last count */
  anem_last = now; 
  if(thisTime > 500)
  {
    anemRotations++;
    // if this interval is shorter than previous, store that.
    if(thisTime<anem_min)
    {
      anem_min=thisTime;
    }
    if(thisTime > anem_max) // and record the minumum gust 
    {
      anem_max = thisTime;
    }
  }
}

static int vaneValues[] PROGMEM={66,84,92,127,184,244,287,406,461,600,631,702,786,827,889,946};
static int vaneDirections[] PROGMEM={1125,675,900,1575,1350,2025,1800,225,450,2475,2250,3375,0,2925,3150,2700};
 
double getWindVane()
{
  analogReference(DEFAULT);
  digitalWrite(VANE_PWR,HIGH);
  delay(100);
  for(int n=0;n<10;n++)
  {
    analogRead(VANE_PIN);
  }
 
  unsigned int reading=analogRead(VANE_PIN);
  digitalWrite(VANE_PWR,LOW);
  unsigned int lastDiff=2048;
 
  for (int n=0;n<16;n++)
  {
    int diff=reading-pgm_read_word(&vaneValues[n]);
    diff=abs(diff);
    if(diff==0)
       return pgm_read_word(&vaneDirections[n])/10.0;
 
    if(diff>lastDiff)
    {
      return pgm_read_word(&vaneDirections[n-1])/10.0;
    }
 
    lastDiff=diff;
 }
 
  return pgm_read_word(&vaneDirections[15])/10.0;
 
}

#define MM_PER_TIP (0.2794 * 100) // take account of th
 
volatile unsigned long rain_count=0;
volatile unsigned long rain_last=0;
volatile unsigned long shortestRainInterval = 0xffffffff;
volatile unsigned long longestRainInterval = 0;


/* get the rain tips since last */ 
struct RainData getUnitRain(unsigned long timeGap)
{
  struct RainData data;
  unsigned long rainBucketTips=rain_count;
  data.rainMM =rainBucketTips * MM_PER_TIP;
  return data;
}

volatile unsigned long lastValidRainTime =0;
void rainGageClick()
{
    long now = micros();
    long thisTime= now - rain_last;
    rain_last = now;
    if(thisTime>500) // only if it's a valid time
    {
      rain_count++;
      unsigned long interval = now - lastValidRainTime;
      if (interval > longestRainInterval)
      {
         longestRainInterval = interval;
      }
      else if (interval < shortestRainInterval)
      {
        shortestRainInterval = interval;
      }
    }
}

void setupSleepMode()
{
#if defined(__AVR_ATtiny84__)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode
  sleep_mode(); 
#endif 
}
 

void setup() {
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power

  rfSetup();
  setupWeatherInts();
  setupSleepMode();
}
volatile unsigned long lastReadTime = 0;
//=======================================================
// Main loop.
//=======================================================
void loop() {
  // this is the main loop. 

  unsigned long now = micros();
  unsigned long timeDiff = (now > lastReadTime)?now - lastReadTime:lastReadTime - now;
  unsigned long timeToSleep = 60000;
  //  only send back data every minute 
  if(timeDiff >= timeToSleep)
  {
   // first of all we get the units rain. in mm.   
    RainData rd = getUnitRain(timeDiff); // Send mm per tip as an integer (multiply by 0.01 at receiving end)
    tinytx.rain = rd.rainMM; // actually it's 100x mm
    
    WindData wd = getAvgWindspeed(timeDiff);  
    tinytx.avgWindSpeed = (int)(wd.avgSpeed*100);
    
    tinytx.windDir = 0;
    tinytx.windGustMax = 0;
      
    tinytx.supplyV = readVcc(); // Get supply voltage
  
    rfwrite(); // Send data via RF
    
    lastReadTime = now;
  }
  else
  {
    timeToSleep -= timeDiff;
  }
  Sleepy::loseSomeTime(timeToSleep);
  
  setupSleepMode();
}
