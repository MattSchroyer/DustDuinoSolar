// Sleeps the Atmega328p for 3 minutes, then wakes and turnes on dust sensor
// Sleeps and wakes Xbee to transmit data after 1m of reading dust sensor
// Additionally measures voltage of input load, before buck/boost
// Xbee transmits the width of low pulses on P1 and P2, in integers, and voltage

#include <avr/sleep.h>
#include <avr/wdt.h>

boolean triggerP1 = false;
boolean triggerP2 = false;
boolean valP1 = HIGH;
boolean valP2 = HIGH;

unsigned long durationP1 = 0;
unsigned long durationP2 = 0;
unsigned long pulseLengthP1;
unsigned long pulseLengthP2;
unsigned long sampletime_ms = 60000;
unsigned long starttime;
unsigned long triggerOnP1;
unsigned long triggerOffP1;
unsigned long triggerOnP2;
unsigned long triggerOffP2;

int volts = 0;

// watchdog interrupt
ISR (WDT_vect)
{
  wdt_disable(); // disable watchdog
}

void setup()
{
  Serial.begin(9600);
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  
  // Digital pin 7 turns the sensor on and off
  pinMode(7, OUTPUT);
  
  // Analog pin 5 reads the battery voltage
  pinMode(A5, INPUT);
  
  //Digital 4 activates Xbee
  //Turns Xbee on to send 0s first, providing proof that the Xbee is within range.
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  
  //Wait for Xbee to wake for 10s
  delay (10000);
      
  Serial.print('A');
  // A for "All particles," from P1 input
  Serial.print(durationP1);
  // Sends just the ratio for P1
  Serial.print('C');
  // C for "Coarse particles," from P2 input
  Serial.print(durationP2);
  // Sends just the ratio for P2
  Serial.print('V');
  // Sends voltage
  Serial.print(volts);
  // E to end the string
  Serial.println('E');

  delay (100);
      
  // Turn Xbee off
  pinMode(4, INPUT);
  digitalWrite(4, HIGH);
      
}

void loop()
{
    // Enter sleep mode for 2m 48s
    for (int i = 1; i < 22; i++)
    {
      enterSleep(0b100001);  // 8 seconds
    }
    enterSleep(0b100000);
    // Sample dust levels
    sampleDust();
    // Send dust levels
    sendDust();

}

void sampleDust(void){
  // Turns on the dust sensor
  digitalWrite(7, HIGH);
  // Wait 1m for dust sensor to settle
  for (int j = 1; j < 11; j++)
  {
    delay(5000);
    wdt_reset();
  }
  
  // Reset low pulse duration
  durationP1 = 0;
  durationP2 = 0;
  
  // Reset various triggers
  triggerP1 = false;
  triggerP2 = false;
  valP1 = HIGH;
  valP2 = HIGH;
  
  // Start the sampling clock
  starttime = millis();
  
  // While the sampling clock is less than the sample time, the loop
  // will continue to read digital pins 3 and 2. Every time a pin goes
  // from low to high, or high to low, the time is recorded and the trigger
  // is reset. Duration of the low pulses is calculated by subtracting
  // the time of the previous trigger to the current time.
  while ((millis() - starttime) < sampletime_ms)
    {
      valP1 = digitalRead(3);
      // P1, which measures all particles, is digital pin 3  
      valP2 = digitalRead(2);
      // P2, which measures only coarse particles, is digital pin 2
    
      if(valP1 == LOW && triggerP1 == false){
        triggerP1 = true;
        triggerOnP1 = micros();
        }
  
      if (valP1 == HIGH && triggerP1 == true){
        triggerOffP1 = micros();
        pulseLengthP1 = triggerOffP1 - triggerOnP1;
        durationP1 = durationP1 + pulseLengthP1;
        triggerP1 = false;
        }
  
      if(valP2 == LOW && triggerP2 == false){
        triggerP2 = true;
        triggerOnP2 = micros();
        }
  
      if (valP2 == HIGH && triggerP2 == true){
        triggerOffP2 = micros();
        pulseLengthP2 = triggerOffP2 - triggerOnP2;
        durationP2 = durationP2 + pulseLengthP2;
        triggerP2 = false;
        }
 
      wdt_reset();
    }
    
    // if in a low state, add to low pulse occupancy.
    
    /*
    if(valP1 == LOW){
        triggerOffP1 = micros();
        pulseLengthP1 = triggerOffP1 - triggerOnP1;
        durationP1 = durationP1 + pulseLengthP1;
    }
    
        if(valP1 == LOW){
        triggerOffP1 = micros();
        pulseLengthP1 = triggerOffP1 - triggerOnP1;
        durationP1 = durationP1 + pulseLengthP1;
    }
    */
}

// Sends the total low pulse duration to Xbee
void sendDust(){
        // Turn dust sensor off
        digitalWrite(7, LOW);
        // Wait for power to settle
        delay (100);
      
        // Turn Xbee on
        pinMode(4, OUTPUT);
        digitalWrite(4, LOW);
  
        //Wait for Xbee to wake for 10s
        wdt_reset();
        delay (5000);
        wdt_reset();
        delay (5000);
        wdt_reset();
      
        volts = analogRead(5);
      
        Serial.print('A');
        // A for "All particles," from P1 input
        Serial.print(durationP1);
        // Sends just the ratio for P1
        Serial.print('C');
        // C for "Coarse particles," from P2 input
        Serial.print(durationP2);
        // Sends just the ratio for P2
        Serial.print('V');
        // Sends voltage of battery in analog value
        Serial.print(volts);
        // E to end the string
        Serial.println('E');
        // Allow message to finish sending
        delay (100);
        // Turn Xbee off
        pinMode(4, INPUT);
        digitalWrite(4, HIGH);
        wdt_reset();
}

void enterSleep(const byte interval)
{
  MCUSR = 0;                          // reset various flags
  WDTCSR |= 0b00011000;               // see docs, set WDCE, WDE
  WDTCSR =  0b01000000 | interval;    // set WDIE, and appropriate delay

  wdt_reset();
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_mode();            // now goes to Sleep and waits for the interrupt
  
  // sleep bit patterns:
  //  1 second:  0b000110
  //  2 seconds: 0b000111
  //  4 seconds: 0b100000
  //  8 seconds: 0b100001
}
