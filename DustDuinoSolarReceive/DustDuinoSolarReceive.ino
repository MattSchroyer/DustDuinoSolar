// Sketch for base station. Receives low pulse occupancy from solar unit.

#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Xively.h>

// MAC address for your Ethernet shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Your Xively key to let you upload data
char xivelyKey[] = "yourxivelykey";

// Define the strings for our datastream IDs
char PM10Conc[] = "PM10";
char PM25Conc[] = "PM25";
char PM10count[] = "PM10count";
char PM25count[] = "PM25count";
char Voltage[] = "Voltage";

float ratioP1 = 0;
float ratioP2 = 0;
float volts = 1.1;
unsigned int samplePeriod = 60000;

XivelyDatastream datastreams[] = {
  XivelyDatastream(PM10Conc, strlen(PM10Conc), DATASTREAM_INT),
  XivelyDatastream(PM25Conc, strlen(PM25Conc), DATASTREAM_INT),
  XivelyDatastream(PM10count, strlen(PM10count), DATASTREAM_INT),
  XivelyDatastream(PM25count, strlen(PM25count), DATASTREAM_INT),
  XivelyDatastream(Voltage, strlen(Voltage), DATASTREAM_FLOAT),
};
// Finally, wrap the datastreams into a feed
XivelyFeed feed(/*your feednumber*/, datastreams, 5 /* number of datastreams */);

EthernetClient client;
XivelyClient xivelyclient(client);

// the possible states of the state-machine
typedef enum {  NONE, GOT_A, GOT_C, GOT_V, GOT_E } states;
// current state-machine state
states state = NONE;
// current partial number
unsigned long currentValue;

void setup() {
  //Setup code will run once
  Serial.begin(9600);
  
  state = NONE;

  while (Ethernet.begin(mac) != 1)
  {
    delay(15000);
  }
}

void processP1 (const unsigned long value)
{
  if (value > 0){
  ratioP1 = value/(samplePeriod * 10.0);
  }
  else{
  ratioP1 = 0;
  }
}

void processP2 (const unsigned long value)
{
  if (value > 0){
  ratioP2 = value/(samplePeriod * 10.0);
  }
  else{
  ratioP2 = 0;
  }
}

void processV (const unsigned long value)
{
  //volts = ((value * 2) / 1023) * 5.0;
  volts = value;
  volts = volts * 2;
  volts = volts / 1023;
  volts = volts * 5;
}

void processPut ()
{
  double countP1 = 1.1*pow(ratioP1,3)-3.8*pow(ratioP1,2)+520*ratioP1+0.62;
  double countP2 = 1.1*pow(ratioP2,3)-3.8*pow(ratioP2,2)+520*ratioP2+0.62;
  countP1 = countP1 - countP2;
      
  if(countP1 < 0){
        countP1 = 0;
  }
  
  // converts particle counts into concentration
  // first, PM10 conversion
  double r10 = 2.6*pow(10,-6);
  double pi = 3.14159;
  double vol10 = (4/3)*pi*pow(r10,3);
  double density = 1.65*pow(10,12);
  double mass10 = density*vol10;
  double K = 3531.5;
  int concLarge = (countP2)*K*mass10;
  
  // next, PM2.5 conversion
  double r25 = 0.44*pow(10,-6);
  double vol25 = (4/3)*pi*pow(r25,3);
  double mass25 = density*vol25;
  int concSmall = (countP1)*K*mass25;
  
  int countLarge = countP2;
  int countSmall = countP1;
      
  //setting datastream values;
  datastreams[0].setInt(concLarge);
  datastreams[1].setInt(concSmall);
  datastreams[2].setInt(countLarge);
  datastreams[3].setInt(countSmall);
  datastreams[4].setFloat(volts);

  int ret = xivelyclient.put(feed, xivelyKey);
}

void handlePreviousState ()
{
  switch (state)
  {
  case GOT_A:
    processP1 (currentValue);
    break;
  case GOT_C:
    processP2 (currentValue);
    break;
  case GOT_V:
    processV (currentValue);
    break;
  case GOT_E:
    processPut ();
    break;
  }  // end of switch  

  currentValue = 0; 
}  // end of handlePreviousState

void processIncomingByte (const byte c)
{
  if (isdigit (c))
  {
    currentValue *= 10;
    currentValue += c - '0';
  }  // end of digit
  else 
  {

    // The end of the number signals a state change
    handlePreviousState ();

    // set the new state, if we recognize it
    switch (c)
    {
    case 'A':
      state = GOT_A;
      break;
    case 'C':
      state = GOT_C;
      break;
    case 'V':
      state = GOT_V;
      break;
    case 'E':
      state = GOT_E;
      break;
    default:
      state = NONE;
      break;
    }  // end of switch on incoming byte
  } // end of not digit  
  
} // end of processIncomingByte


void loop() {
  if (Serial.available ())
    processIncomingByte (Serial.read ());
    
}

