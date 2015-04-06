
/* La Fabrique

   RFID Link Test
   
   by David Beaudette
   
   Code that answers commands from the access server.
   
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "nano_rfid_hal.h"

// Declare radio
RF24 radio(radioCePin, radioCsnPin);

void setup() {
  // Reset digital outputs
  SetPins();

  SPI.begin();
  Serial.begin(serialRate);
  printf_begin();
  
  Serial.println("RFID_Link Test");
  Serial.println("Configuring NRF24L01+.");
  
  RadioConfig(&radio);
  
  Serial.println("Started listening.");
  
  // Signal self-test success
  FlashLed(grnLedPin, quickFlash, 3);
}

void loop() {
  
  // Check for received packet
  if (radio.available())
  {
    // Dump the payloads until we've gotten everything
    uint8_t rx_buf[7] = {0,0,0,0,0,0,0};;
    uint8_t tx_buf[9] = {0xAF,0,0,0,0,0,0,0,0};
    
    int table_count = 0;

    // Fetch the payload, and see if this was the last one.
    radio.read(&rx_buf[0], 1);
    
    // Delay just a little bit to let the other unit
    // make the transition to receiver
    delay(20);
  
    // Treat the table update command differently: command is 
    // accompanied with a table entry we have to reply quickly
    if(rx_buf[0] == 0xA4) {
      radio.read(&rx_buf[0], 7);
      table_count = rx_buf[1];
      for (int i = 1; i < table_count; i++) {
        
        radio.stopListening();
        tx_buf[1] = 0xA4;
        tx_buf[2] = 0xD0 + (i % 3) + 1;
        if(!radio.write(&tx_buf[0], 3)) 
            printf("Unable to send back data.\n\r");  
          
        radio.startListening();
        // Read next table entry
        while(!radio.available()) {delay(1);}
        radio.read(&rx_buf[0], 7);
        delay(20);
      }
      // Reply for last entry
      radio.stopListening();
      tx_buf[1] = 0xA4;
      tx_buf[2] = 0xD0 + (table_count % 2) + 1;
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
        printf("Received dump logging command.\n\r");
        tx_buf[1] = 0xA3;
        if(!radio.write(&tx_buf[0], 9)) 
          printf("Unable to send back data.\n\r");  
        break;  
        
      case 0xA4:        
        printf("Received table update command.\n\r");
        printf("  There were %d entries to update.\n\r", table_count);
        printf("  Last user is authorised if %d == 1.\n\r", rx_buf[2]);
        printf("  Last user code is %d, %d, %d, %d.\n\r", rx_buf[3], rx_buf[4], rx_buf[5], rx_buf[6]);
        break;  
        
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
  // Wait before checking for another packet
  FlashLed(grnLedPin, slowFlash, 1);
  delay(slowFlash);
}

