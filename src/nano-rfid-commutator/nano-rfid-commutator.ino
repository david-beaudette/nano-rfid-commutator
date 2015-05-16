/* La Fabrique

   RFID Commutator on Arduino Nano 
   by David Beaudette
   
   Code that answers commands from the access server and
   detects proximity cards.
   
*/

#include <SPI.h>
#include <EEPROM.h>

#include <MFRC522.h>
#include <Wiegand.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Event.h>
#include <Timer.h>

#include "nano_rfid_hal.h"
#include "AccessEvent.h"
#include "AccessTable.h"
#include "LinkCommand.h"

// Define parameters that are proper to the programmed unit
const uint8_t radioChannel = 3;

// Declare radio
RF24 radio(radioCePin, radioCsnPin);

// Declare card readers
MFRC522 nfc(rfSdaPin, rfResetPin);
WIEGAND wg;

// Declare timer object and global counter
Timer t;
// Using implicit zero-initialisation  
unsigned long EventList::event_list_counter;

// Declare table of events
EventList eventList(&t, 50);

// Card tag read flag
int read_tag_flag = 1;

// Declare table of users
AccessTable table;

// Declare commmand manager
LinkCommand link(&table, &eventList);

// State variable
// System state variable
sys_state_t state;  
act_mode_t act_mode; 
byte first_act_tag[4]; 

void setup() {
  SetPins();

  SPI.begin();
  Serial.begin(115200);
  
  Serial.println("nano-rfid-commutator");
  Serial.println("Configuring NRF24L01+.");
  RadioConfig(&radio, radioChannel);
  
  Serial.println("Configuring MFRC522.");
  if (nfc.digitalSelfTestPass()) {
    Serial.println("Digital self test by MFRC522 passed.");
  } else {
    Serial.println("Digital self test by MFRC522 failed.");
    Stall();
  }  
  // Signal self-test success
  FlashLed(grnLedPin, quickFlash, 3);
  
  // Initialise card readers
  nfc.begin();
  wg.begin();
 
  // Set initial state
  state = IDLE;
  act_mode = SINGLE;
  
  // Create an event to flash green led every minute
  int dumpEvent = t.every(30000, BlinkOk);
  
  // Print initial table content
  table.print_table();
}

void loop() {
  // Detected card type and tag
  byte type[2];
  byte tag[4];
  byte tag_read_len;
  
  // Initialise command buffer with 8 bytes, which 
  // is 1 more than the longest command (table update)
  byte cmd[7];
  byte cmd_len;
  
  // Processed length: if smaller than command length
  // more than one command was probably received in 
  // one packet
  int  prc_len;
  
  // Initialise reply buffer with 10 bytes, which 
  // is 1 more than the longest reply (dump logging)
  byte reply[10];
  byte reply_len;
  
  // Update timer
  t.update();
    
  // Check for received packet
  if(radio.available())
  {
    // Check number of bytes received
    cmd_len = radio.getDynamicPayloadSize();
    if(cmd_len > 0)
    {   
      // Read all command bytes from the receive buffer
      radio.read(&cmd, cmd_len);
      
      // Flush buffers and switch to TX mode
      radio.stopListening();
      
      // Let LinkCommand handle this command
      prc_len = link.processCommand(cmd, reply, 
                                    &reply_len, 
                                    &state,
                                    &act_mode);
      if(prc_len == -1) {
        // Signal error in processing
        Serial.println("Error processing received command.");
        FlashLed(redLedPin, slowFlash, 5);
        return;
      }
      // Reply to sender
      if(!radio.write(reply, reply_len))   {
        printf("Unable to reply to sender.\n\r");
      }        
      // Check if received packet contained another command
      if(prc_len < cmd_len) {
        Serial.println("At least one command was ignored in the packet...");
      }
      // Return to receiving state
      radio.startListening();
    }
  }
  // Check for proximity card
  if(state > DISABLED) {
    // Neither in enabled nor disabled states
    // Check if we are ready to read again
    if(read_tag_flag > 0) {  
      // Check for card
      tag_read_len = 0;
      if(nfc.readSerial(tag, type) > 0) {
        tag_read_len = 4;
      }
      if(wg.available()) {
        // Wiegand reader only reads 3/4 bytes
        wg.getCode(tag);
        tag_read_len = 3;
      }
      // Check if one of the two readers has something
      if(tag_read_len > 0) {  
        // Check if user is authorized
        switch(table.getUserAuth(tag, tag_read_len)) {
          case -1:
            // User not found: add unknown logging event
            eventList.addEvent(0x34, tag);
            FlashLed(redLedPin, quickFlash, 2);
            break;
          case 0:
            // User not authorized
            eventList.addEvent(0x33, tag);
            FlashLed(redLedPin, quickFlash, 1);
            break;
          case 1:
            // User authorized
            switch(state) {
              case ACTIVATED:
                // User logs out
                eventList.addEvent(0x32, tag);
                state = IDLE;
                break;
              case IDLE:
                if(act_mode == DOUBLE) {
                  // First user logs in
                  eventList.addEvent(0x30, tag);
                  state = TRIGGEREDONCE;
                  memcpy(first_act_tag, tag, 4*sizeof(byte));
                  t.after(5000, ResetFirstTag);
                }
                else {
                  // Enable relay                  
                  eventList.addEvent(0x31, tag);
                  state = ACTIVATED;
                }
              case TRIGGEREDONCE:
                  // Second user logs, enable relay
                  int idx = 0;
                  while(idx < 4 && 
                        first_act_tag[idx] == tag[idx]) {
                    idx++;
                  }
                  if(idx == 4) {
                    // Same user logged twice
                    eventList.addEvent(0x33, tag);
                    FlashLed(redLedPin, quickFlash, 1);
                  } 
                  else {
                    eventList.addEvent(0x31, tag);
                    state = ACTIVATED;                    
                  }
                  break;
            }
            break;
        }
        // Wait 1 second before reading again
        read_tag_flag = 0;
        t.after(1000, SetReadFlag);
      }
    }       
  }
  // Set relay and LEDs
  if(state == ACTIVATED || state == ENABLED) {
    // Activate relay
    digitalWrite(relayPin,  HIGH);
    digitalWrite(redLedPin, LOW);
    digitalWrite(grnLedPin, HIGH);
  }
  else if(state == DISABLED) {
    // Activate relay
    digitalWrite(relayPin,  LOW);
    digitalWrite(grnLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
  }
  else {
    // Deactivate relay
    digitalWrite(relayPin,  LOW);
    digitalWrite(grnLedPin, LOW);
    digitalWrite(redLedPin, LOW);
  }
  delay(loopRate);
}

void SetReadFlag() {
  read_tag_flag = 1;
}

void ResetFirstTag() {
  state = IDLE;
}

void BlinkOk() {
  if(state != ACTIVATED && state != ENABLED) {
    FlashLed(grnLedPin, quickFlash, 2);
  }
}
