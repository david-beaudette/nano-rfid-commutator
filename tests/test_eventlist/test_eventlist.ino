/* La Fabrique

   RFID Commutator on Arduino Nano 
   by David Beaudette
   
   Tests the EventList class methods with the real 
   card reader. Also tests the Timer class usage for
   the final app.
   
*/
#include <SPI.h>

#include <Event.h>
#include <Timer.h>

#include <MFRC522.h>
#include "AccessEvent.h"

void FlashLed(const int pinNum, const int duration_ms, int num_times = 1);
void Stall();
void DumpLogging();

// GPIO pin numbers
const int redLedPin   = 3;      // Red LED
const int grnLedPin   = 4;      // Green LED
const int relayPin    = 7;      // Relay
const int rfResetPin  = 9;      // RC522 reset signal
const int rfSdaPin    = 10;     // RC522 SDA signal

// Declare card reader
MFRC522 nfc(rfSdaPin, rfResetPin);

// Declare timer object and global counter
Timer t;

// Using implicit zero-initialisation  
unsigned long EventList::event_list_counter;

// Declare list of events
EventList eventList(&t, 50);

const int quickFlash = 500;    // duration in ms for quickly flashing a LED
const int slowFlash  = 1000;   // duration in ms for slowly flashing a LED

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
  
  Serial.println("EventList and RFID test");
  Serial.println("---------");

  Serial.println("Configuring MFRC522.");
  if (nfc.digitalSelfTestPass()) {
    Serial.println("Digital self test by MFRC522 passed.");
  } else {
    Serial.println("Digital self test by MFRC522 failed.");
    Stall();
  }  
  // Create an event to dump logging to screen every 20 s
  int dumpEvent = t.every(20000, DumpLogging);
  
  // Create a LED flashing thread
  int ledFlash = t.oscillate(grnLedPin, 1000, LOW);
  
  // Signal self-test success
  FlashLed(grnLedPin, quickFlash, 3);
  
  nfc.begin();
  
}

void loop() {
  // Event type will cycle through the possible 
  // Type is one of the following:
  //  0x30 : ‘Attempt’ (first authorization in double authorization mode).
  //  0x31 : ‘Confirm’ (authorized user activated relay).
  //  0x32 : ‘Logout’  (authorized user deactivated relay).
  //  0x33 : ‘Fail’    (unauthorized user card detected).
  //  0x34 : ‘Unknown’ (unknown user card detected).
  int event_type = 0x30;
  
  byte card_status;
  byte data[MAX_LEN];
  byte tag[5];
  
  // Update  timer
  t.update();
  
  // Check for cards, log events
  card_status = nfc.requestTag(MF1_REQIDL, data);

  if (card_status == MI_ERR) {
    Serial.println("Error reading card.");  
    Stall();  
  }
  else if (card_status == MI_NOTAGERR) {
    Serial.println("No tag detected.");
  }
  else if (card_status == MI_OK) {
    Serial.println("Tag detected.");
    Serial.print("Type: ");
    Serial.print(data[0], HEX);
    Serial.print(", ");
    Serial.println(data[1], HEX);

    // calculate the anti-collision value for the currently detected
    // tag and write the serial into the data array.
    card_status = nfc.antiCollision(data);

    if (card_status == MI_ERR) {
      Serial.println("Error in serial checksum.");    
    } 
    else {
      memcpy(tag, data, 5);  
    }
    // Stop the tag and get ready for reading a new tag.
    nfc.haltTag();
    
    // Add an event to the list
    eventList.addEvent(event_type, tag);    
    event_type = (0x34 == event_type) ? 0x30 : (event_type+1);
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

void DumpLogging() {
  AccessEvent *event;
  int list_size = eventList.getListSize();
  Serial.print("Dumping events; list size is ");
  Serial.println(list_size);
  for(int i = 0; i < list_size; i++) {
    event = eventList.getEvent();
    Serial.println("---------");
    event->ToSerial();
  }    
  Serial.println("---------");
}

