
/* La Fabrique

   RFID Link Test
   
   by David Beaudette
   
   Code that answers commands from the access server.
   
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

// GPIO pin numbers
const int redLedPin   = 3;      // Red LED
const int grnLedPin   = 4;      // Green LED
const int radioCsnPin = 5;      // NRF24L01+ CSN signal
const int radioCePin  = 6;      // NRF24L01+ CE signal
const int relayPin    = 7;      // Relay
const int rfResetPin  = 9;      // RC522 reset signal
const int rfSdaPin    = 10;     // RC522 SDA signal

// Radio configuration
RF24 radio(radioCePin, radioCsnPin);
const uint64_t pipes[2]    = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
const uint8_t radioChannel = 10;

// Other program constants
const int quickFlash = 500;    // duration in ms for quickly flashing a LED

// Declare radio

void setup() {
  // Reset digital outputs
  pinMode(relayPin,  OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(grnLedPin, OUTPUT);
  
  digitalWrite(relayPin,  LOW);
  digitalWrite(redLedPin, LOW);
  digitalWrite(grnLedPin, LOW);

  SPI.begin();
  Serial.begin(57600);
  printf_begin();
  printf("\n\rRFID_Link/Test/\n\r");

  Serial.println("Looking for NRF24L01+.");
  radio.begin();
  Serial.println("Radio began.");
  radio.setRetries(15,15);
  Serial.println("Retry config set.");
  radio.setPayloadSize(8);
  Serial.println("Payload size set.");
  radio.openWritingPipe(pipes[1]);
  Serial.println("Opened writing pipe.");
  radio.openReadingPipe(1,pipes[0]);
  Serial.println("Opened reading pipe.");
  
  radio.startListening();
  Serial.println("Started listening.");
  radio.setChannel(0x4c);
  radio.enableDynamicPayloads();
  radio.setAutoAck(true);
  radio.setDataRate(RF24_1MBPS);
  radio.printDetails();
}

void loop() {
  
  //Serial.println("Looping.");
  // Check for received packet
  if (radio.available())
  {
    // Dump the payloads until we've gotten everything
    unsigned long rx_buf;
    bool done = false;
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      done = radio.read(&rx_buf, sizeof(unsigned long));
      printf("Got payload %lu...", rx_buf);
  
      // Delay just a little bit to let the other unit
      // make the transition to receiver
      delay(20);
    }
  
    // First, stop listening so we can talk
    //radio.stopListening();
  
    // Send the final one back.
    //radio.write(&rx_buf, sizeof(unsigned long));
    //printf("Sent response.\n\r");
  
    // Now, resume listening so we catch the next packets.
    //radio.startListening();
  }
  // Wait before checking for another packet
  //delay(1000);
}

