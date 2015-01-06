
/* La Fabrique

   RFID Activate
   
   by David Beaudette
   
   Code that reads RFID card and activates the card.
   
*/

#include "LIB_MFRC522.h"
#include <SPI.h>
#include <avr/pgmspace.h>

const int relayPin = 7;      // the number of the LED pin
#define SAD 10
#define RST 9

#define READER_NUMBER B10000001     // ID for this device (starts with 1)

// Variables that communicate with reader
const byte mode = 0; // communication mode
byte data[100];      // data buffer
int status;          // return status for function calls
int state;           // one the following state:  
                     // 0: Idle
                     // 1: Activated
                     // 2: Timeout
                     // 3: Disabled
                     // 4: Enabled
                     // 5: Triggered once

// Data would normally go in EEPROM
prog_uchar const authKeys[1024] PROGMEM = {0xB3, 0xED, 0xE6, 0xC7,
                                           0x5D, 0x84, 0xF7, 0x54, 
                                           0x70, 0x40, 0x84, 0x0B};
int  numKeys = 0;

MFRC522 nfc(SAD, RST);


void setup() {
  // Set relay digital pin as output
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  SPI.begin();
  Serial.begin(115200);

  Serial.println("Looking for MFRC522.");
  if (nfc.digitalSelfTestPass()) {
      Serial.println("Digital self test by MFRC522 passed.");
  } else {
      Serial.println("Digital self test by MFRC522 failed.");
  }

  uint8_t version = nfc.getFirmwareVersion();
  if (! version) {
    Serial.print("Didn't find MFRC522 board.");
    while(1); //halt
  }

  Serial.print("Found chip MFRC522 ");
  Serial.print("Firmware ver. 0x");
  Serial.print(version, HEX);
  Serial.println(".");
  
  nfc.begin();
  
  // Set state as idle
  state = 0;
  
  // For now update authorized keys table with hardcoded values
  numKeys      = 3;
  
}

void loop() {
  byte status;
  byte data[MAX_LEN];
  byte serial[5];
  int i, j, pos, curKey, tableIdx;

  // Send a general request out into the aether. If there is a tag in
  // the area it will respond and the status will be MI_OK.
  status = nfc.requestTag(MF1_REQIDL, data);

  if (status == MI_ERR) {
    Serial.println("Error reading card.");    
  }
  else if (status == MI_OK) {
    Serial.println("Tag detected.");
    Serial.print("Type: ");
    Serial.print(data[0], HEX);
    Serial.print(", ");
    Serial.println(data[1], HEX);

    // calculate the anti-collision value for the currently detected
    // tag and write the serial into the data array.
    status = nfc.antiCollision(data);

    if (status == MI_ERR) {
      Serial.println("Error in serial checksum.");    
    } else {
      memcpy(serial, data, 5);
  
      Serial.println("The serial nb of the tag is:");
      for (i = 0; i < 3; i++) {
        Serial.print(serial[i], HEX);
        Serial.print(", ");
      }
      Serial.println(serial[3], HEX);
      tableIdx = -1;
      // Check if this tag is authorized
      Serial.println("Comparing keys.");
      for(curKey = 0; curKey < numKeys; curKey++) {
        Serial.print("Comparing key #");
        Serial.print(curKey);
        Serial.print(" with serial number ");
        for (i = 0; i < 3; i++) {
          Serial.print(pgm_read_byte_near(authKeys + curKey*4 + i), HEX);
          Serial.print(", ");
        }
        Serial.println(pgm_read_byte_near(authKeys + curKey*4 + 3), HEX);
        i = 0;
        while(pgm_read_byte_near(authKeys + curKey*4 + i) == serial[i] && i < 4) {
           i++;
        }
        if(i == 4) {
          // All bytes compared equal
          tableIdx = curKey;
          break;
        }
      }     
      if(tableIdx >= 0) {
        switch(state) {
          case 0:
            // Activate relay
            digitalWrite(relayPin, HIGH);
            Serial.println("Authorized. Activating relay.");    
            state = 1;
            delay(2000);
            break;
          default:
            // Deactivate relay
            digitalWrite(relayPin, LOW);
            Serial.println("Authorized. De activating relay.");    
            state = 0;
            delay(2000);
        }
      } 
    }
    // Stop the tag and get ready for reading a new tag.
    nfc.haltTag();
  } 
  
  delay(1000);
}
