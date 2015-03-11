/* La Fabrique

   RFID Commutator on Arduino Nano 
   by David Beaudette
   
   Code that answers commands from the access server and
   detects proximity cards.
   
*/

#include <SPI.h>
#include <EEPROM.h>

#include <MFRC522.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Event.h>
#include <Timer.h>

#include "AccessEvent.h"
#include "AccessTable.h"
#include "LinkCommand.h"

// GPIO pin numbers
const int redLedPin   = 3;      // Red LED
const int grnLedPin   = 4;      // Green LED
const int radioCsnPin = 5;      // NRF24L01+ CSN signal
const int radioCePin  = 6;      // NRF24L01+ CE signal
const int relayPin    = 7;      // Relay
const int rfResetPin  = 9;      // RC522 reset signal
const int rfSdaPin    = 10;     // RC522 SDA signal

// Radio configuration
const uint64_t pipes[2]    = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
const uint8_t radioChannel = 0x4c;
const uint8_t payloadSize  = 10;
const int retryDelay       = 15;
const int retryCount       = 15;
const rf24_datarate_e dataRate = RF24_1MBPS;

// Other program constants
const int quickFlash = 500;    // duration in ms for quickly flashing a LED
const int slowFlash  = 1000;   // duration in ms for slowly flashing a LED

// Declare radio
RF24 radio(radioCePin, radioCsnPin);

// Declare card reader
MFRC522 nfc(rfSdaPin, rfResetPin);

// Declare timer object
Timer t;
unsigned long eventTime = 0UL;

// Declare table of events
AccessEvent events[MAX_NUM_EVENTS];
int curNumEvents = 0;

// Declare table of users
AccessTable table;

// Declare commmand manager
LinkCommand link(&radio, &table, &events[0]);

// State variable
int state;           // one the following state:  
                     // 0: Idle
                     // 1: Activated
                     // 2: Timeout
                     // 3: Disabled
                     // 4: Enabled
                     // 5: Triggered once

void setup() {
  // Reset digital outputs
  pinMode(relayPin,  OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(grnLedPin, OUTPUT);
  
  digitalWrite(relayPin,  LOW);
  digitalWrite(redLedPin, LOW);
  digitalWrite(grnLedPin, LOW);

  SPI.begin();
  Serial.begin(115200);
  
  Serial.println("nano-rfid-commutator");
  Serial.println("Configuring NRF24L01+.");
  
  radio.begin();
 // Configure as slave in the communication protocol
  radio.setRetries(retryDelay, retryCount);
  radio.setPayloadSize(payloadSize);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.setChannel(radioChannel);
  radio.enableDynamicPayloads();
  radio.setAutoAck(true);
  radio.setDataRate(dataRate);
  
  radio.startListening();
  radio.printDetails();
  
  Serial.println("Configuring MFRC522.");
  if (nfc.digitalSelfTestPass()) {
    Serial.println("Digital self test by MFRC522 passed.");
  } else {
    Serial.println("Digital self test by MFRC522 failed.");
    stall();
  }  
  // Signal self-test success
  flashLed(grnLedPin, quickFlash);
  delay(quickFlash);
  flashLed(grnLedPin, quickFlash);
  delay(quickFlash);
  flashLed(grnLedPin, quickFlash);
  
  nfc.begin();
  
  // Set state as idle
  state = 0;
}

void loop() {
  
  // Check for received packet
  if (radio.available())
  {
    if(!getCommand()) {
      Serial.println("Fatal error with received command.");
      stall();
    }
  }
  // TODO: Check for proximity card
  
  // Wait before checking for another packet
  flashLed(redLedPin, slowFlash);
  delay(slowFlash);
}

void flashLed(const int pinNum, const int duration_ms) {
  digitalWrite(pinNum, HIGH);
  delay(duration_ms);
  digitalWrite(pinNum, LOW);
}

inline void stall() {
    while(1) {
    flashLed(redLedPin, quickFlash);
    delay(quickFlash);
    flashLed(redLedPin, quickFlash);
    delay(2000);
  }
}

int getCommand() {
  // Initialize the message buffers 
  byte rx_buf[7] = {0,0,0,0,0,0,0};
  // tx_buf[0] is the current state of the Arduino
  byte tx_buf[9] = {0xAF,0,0,0,0,0,0,0,0};
  
  // Read the command byte from the RX buffer
  radio.read(&rx_buf[0], 1);
  
  // Delay just a little bit to let the other unit
  // make the transition to receiver
  delay(20);

  // Table update command is accompanied with a table entry
  if(rx_buf[0] == 0xA4) {
    // Initialise reply with proper command
    tx_buf[1] = 0xA4;
    
    // Read the whole buffer sent with command
    radio.read(&rx_buf[0], 7);
    
    int table_count = rx_buf[1];
    for (int i = 1; i < table_count; i++) {
      // Flush buffers and switch to TX mode
      radio.stopListening();
      
      // Update user entry in table
      updateResult = table.setAuth(&rx_buf[3], rx_buf[2]);
      tx_buf[2] = mapTableUpdateResult(updateResult);
      
      // Send result
      if(!radio.write(&tx_buf[0], 3)) 
          printf("Unable to send back data.\n\r");  
        
      // Setup and wait for next table entry
      radio.startListening();
      while(!radio.available()) {delay(1);}
      radio.read(&rx_buf[0], 7);
      delay(20);
    }
    // Reply for last entry
    radio.stopListening();
    // Update last user entry in table
    updateResult = table.setAuth(&rx_buf[3], rx_buf[2]);
    tx_buf[2] = mapTableUpdateResult(updateResult);
    // Send result
    if(!radio.write(&tx_buf[0], 3)) 
      printf("Unable to send back data.\n\r");
  }

  // First, stop listening so we can talk
  radio.stopListening();
  
  switch(rx_buf[0]) {
    case 0xA0:
      printf("Received auto command.\n\r");
      if(!radio.write(&tx_buf[0], 1)) 
        printf("Unable to send back data.\n\r");
      break;
      
    case 0xA1:
      printf("Received enable command.\n\r");
      if(!radio.write(&tx_buf[0], 1)) 
        printf("Unable to send back data.\n\r");  
      break;  
      
    case 0xA2:
      printf("Received disable command.\n\r");
      if(!radio.write(&tx_buf[0], 1)) 
        printf("Unable to send back data.\n\r");  
      break; 
      
    case 0xA3:
      tx_buf[1] = 0xA3;
      if(!radio.write(&tx_buf[0], 9)) 
        printf("Unable to send back data.\n\r");  
      break;  
      
    case 0xA4:
      printf("Received dump logging command.\n\r");
           
    case 0xA5:
      printf("Received check memory command.\n\r");
      tx_buf[1] = 0xA5;
      tx_buf[2] = 0x00;
      tx_buf[3] = 0x04;
      tx_buf[4] = 0xFE; // 510
      tx_buf[5] = 0x01;
      if(!radio.write(&tx_buf[0], 6)) 
        printf("Unable to send back data.\n\r");  
      break;  
      
    case 0xA6:
      printf("Received clear memory command.\n\r");
      if(!radio.write(&tx_buf[0], 1)) 
        printf("Unable to send back data.\n\r");  
      break; 
  }

  // Resume listening so we catch the next command.
  radio.startListening();
}

// Maps the AccessTable::setUserAuth return argument to
// the proper table update command answer
byte mapTableUpdateResult(int retArg) {
  switch(retArg) {
    case -2:
      // Table full
      return 0xDF;
    case -1:
      // New user
      return 0xD3;
    case 0:
      // No update required 
      return 0xD1;
    case 1:
      // Updated authorization
      return 0xD2;
  }
}

