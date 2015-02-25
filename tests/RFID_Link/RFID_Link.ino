
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
const uint64_t pipes[2]    = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
const uint8_t radioChannel = 0x4c;
const uint8_t payloadSize  = 8;
const int retryDelay       = 15;
const int retryCount       = 15;
const rf24_datarate_e dataRate = RF24_1MBPS;

// Other program constants
const int quickFlash = 500;    // duration in ms for quickly flashing a LED
const int slowFlash  = 1000;   // duration in ms for slowly flashing a LED

// Declare radio
RF24 radio(radioCePin, radioCsnPin);

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
  
  Serial.println("RFID_Link Test");
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
  
  Serial.println("Started listening.");
  
  // Signal self-test success
  flashLed(grnLedPin, quickFlash);
  delay(quickFlash);
  flashLed(grnLedPin, quickFlash);
  delay(quickFlash);
  flashLed(grnLedPin, quickFlash);
}

void loop() {
  
  // Check for received packet
  if (radio.available())
  {
    // Dump the payloads until we've gotten everything
    uint8_t rx_buf;
    bool done = false;
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      done = radio.read(&rx_buf, 1);
      //printf("Received %d\r\n", rx_buf);
  
      // Delay just a little bit to let the other unit
      // make the transition to receiver
      delay(20);
    }
  
    // First, stop listening so we can talk
    radio.stopListening();
  
    // Send the received number.
    if(radio.write(&rx_buf, 1)) {
      printf("Received and sent back %d.\n\r", rx_buf);
    }
  
    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
  // Wait before checking for another packet
  flashLed(redLedPin, slowFlash);
  delay(slowFlash);
}

void flashLed(const int pinNum, const int duration_ms) {
  digitalWrite(pinNum, HIGH);
  delay(duration_ms);
  digitalWrite(pinNum, LOW);
}
