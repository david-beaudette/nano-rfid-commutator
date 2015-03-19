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

void TrackTime(void);

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
unsigned long event_list_counter;

// Declare table of events
EventList eventList(&t);

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
  
  Serial.println("test_eventlist");
  
  Serial.println("Configuring MFRC522.");
  if (nfc.digitalSelfTestPass()) {
    Serial.println("Digital self test by MFRC522 passed.");
  } else {
    Serial.println("Digital self test by MFRC522 failed.");
    stall();
  }  
  // Create an event to increment 
  // global counter every second
  int trackEvent = t.every(1000, TrackTime);
  
  // Create an event to dump logging to screen every minute
  int dumpEvent = t.every(60000, DumpLogging);
  
  // Signal self-test success
  flashLed(grnLedPin, quickFlash);
  delay(quickFlash);
  flashLed(grnLedPin, quickFlash);
  delay(quickFlash);
  flashLed(grnLedPin, quickFlash);
  
  nfc.begin();
  
}

void loop() {
  byte cardStatus;
  byte data[MAX_LEN];
  byte serial[5];
  int  i;

  // Event type will cycle through the possible 
  // Type is one of the following:
  //  0x30 : ‘Attempt’ (first authorization in double authorization mode).
  //  0x31 : ‘Confirm’ (authorized user activated relay).
  //  0x32 : ‘Logout’  (authorized user deactivated relay).
  //  0x33 : ‘Fail’    (unauthorized user card detected).
  //  0x34 : ‘Unknown’ (unknown user card detected).
  int  eventType = 0;
  
  // Update  timer
  t.update();
  
  // Check for cards, log events
  cardStatus = nfc.requestTag(MF1_REQIDL, data);

  if (cardStatus == MI_ERR) {
    Serial.println("Error reading card.");    
  }
  else if (cardStatus == MI_OK) {
    Serial.println("Tag detected.");
    Serial.print("Type: ");
    Serial.print(data[0], HEX);
    Serial.print(", ");
    Serial.println(data[1], HEX);

    // calculate the anti-collision value for the currently detected
    // tag and write the serial into the data array.
    cardStatus = nfc.antiCollision(data);

    if (cardStatus == MI_ERR) {
      Serial.println("Error in serial checksum.");    
    } 
    else {
      memcpy(serial, data, 5);  
      PrintSerial(serial);
    }
    // Stop the tag and get ready for reading a new tag.
    nfc.haltTag();
    
    // Add an event to the list
    eventList.addEvent(eventType, serial);    
  }   
  // Wait before checking for another card
  flashLed(redLedPin, slowFlash);
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

void TrackTime() {
  // Increment counter 
  event_list_counter += 1UL;
}

void DumpLogging() {
  AccessEvent *event;
  for(int i = 0; i < eventList.getListSize(); i++) {
    event = eventList.getEvent();
    Serial.println("---------");
    PrintEvent(event);
  }    
}

void PrintEvent(AccessEvent *event) {
  // Print time
  Serial.print("Event happened ");
  Serial.print(event->time);
  Serial.println(" seconds ago.");
  // Print type
  Serial.print("The type of the event is: ");
  Serial.println(event->type, HEX);
  // Print tag id
  PrintSerial(event->tag_id);
}

void PrintSerial(byte *serial) {
  Serial.println("The serial nb of the tag is:");
  for (int i = 0; i < 3; i++) {
    Serial.print(serial[i], HEX);
    Serial.print(", ");
  }
  Serial.println(serial[3], HEX);
}
