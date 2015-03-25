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

// Other program constants
const int quickFlash = 500;    // duration in ms for quickly flashing a LED
const int slowFlash  = 1000;   // duration in ms for slowly flashing a LED

// Declare radio
RF24 radio(radioCePin, radioCsnPin);

// Declare card reader
MFRC522 nfc(rfSdaPin, rfResetPin);

// Declare timer object and global counter
Timer t;
unsigned long event_list_counter;

// Declare table of events
EventList eventList(&t, 50);

// Card tag read flag
int read_tag_flag = 1;

// Declare table of users
AccessTable table;

// Declare commmand manager
LinkCommand *link;

// State variable
int state;  // one the following state:  
            //  0 - Enabled: As commanded by server: relay activated until told otherwise
            //  1 - Disabled: As commanded by server: relay deactivated until told otherwise
            //  2 - Activated: Valid RFID triggered: relay activated until valid RFID triggers
            //  3 - Idle: Relay deactivated, wait for card or server command
            //  4 - TriggeredOnce: In dual RFID activation mode, valid RFID triggered: waiting for a second one

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
  link = new LinkCommand(&radio, &table, &eventList);
  
  Serial.println("Configuring MFRC522.");
  if (nfc.digitalSelfTestPass()) {
    Serial.println("Digital self test by MFRC522 passed.");
  } else {
    Serial.println("Digital self test by MFRC522 failed.");
    Stall();
  }  
  // Signal self-test success
  FlashLed(grnLedPin, quickFlash, 3);
  
  nfc.begin();
  
  // Set initial state as idle
  state = 0;
}

void loop() {
  // Detected card type and tag
  byte type[2];
  byte tag[4];
  
  // Update  timer
  t.update();
  
  // Check for received packet
  if (radio.available())
  {
    if(!link->processCommand(&state)) {
      Serial.println("Fatal error with received command.");
      Stall();
    }
  }
  // TODO: Check for proximity card
  if(state > 1) {
    // Neither in enabled nor disabled states
    // Check if we are ready to read again
    if(read_tag_flag > 0) {  
      // Check for card
      if(nfc.readSerial(tag, type) > 0) {  
        // Check if user is authorized
        
        //   
        // Add an event to the list
        eventList.addEvent(event_type, tag);
        
        // Wait 1 second before reading again
        read_tag_flag = 0;
        t.after(1000, SetReadFlag);
      }
    }       
  }
}

void FlashLed(const int pinNum, const int duration_ms, int num_times) {
  for(int i = 0; i < num_times; i++) {
    digitalWrite(pinNum, HIGH);
    delay(duration_ms);
    digitalWrite(pinNum, LOW);
    if(i < (num_times-1)) {
      delay(duration_ms);
    }
  }
}

void Stall() {
  while(1) {
    FlashLed(redLedPin, quickFlash, 2);
    delay(2000);
  }
}

void SetReadFlag() {
  read_tag_flag = 1;
}

