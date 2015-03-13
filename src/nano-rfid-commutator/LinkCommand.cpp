/*
  LinkCommand.cpp
  
  Defines the list of user ID's and associated authorisations.
 */

#include "LinkCommand.h"

LinkCommand::LinkCommand(RF24 *radio, 
                         AccessTable *table, 
                         EventList *event_list) {
  // Assign class pointers                             
  _radio = radio;
  _table = table;
  _event_list = event_list;  

 // Configure as slave in the communication protocol
  _radio->begin();
  _radio->setRetries(retryDelay, retryCount);
  _radio->setPayloadSize(payloadSize);
  _radio->openWritingPipe(pipes[1]);
  _radio->openReadingPipe(1,pipes[0]);
  _radio->setChannel(radioChannel);
  _radio->enableDynamicPayloads();
  _radio->setAutoAck(true);
  _radio->setDataRate(dataRate);
  
  _radio->startListening();
  _radio->printDetails();
  
}

int LinkCommand::processCommand(int *systemState) { 
  int replyResult;
  
  // Read the command byte from the receive buffer
  byte rx_buf;
  _radio->read(&rx_buf, 1);
  
  switch(rx_buf) {
    case 0xA0:
      // Switch to AUTO mode if enabled or disabled
      if(*systemState < 2) {*systemState = 3;}
      replyResult = this->replyOk();
      break;
      
    case 0xA1:
      // Switch to ENABLED mode
      *systemState = 0;
      replyResult = this->replyOk();
      break;  
      
    case 0xA2:
      // Switch to DISABLED mode
      *systemState = 1;
      replyResult = this->replyOk();
      break;  
      
    case 0xA3:
      // DUMP LOGGING
      //tx_buf[1] = 0xA3;
      //if(!radio.write(&tx_buf[0], 9)) 
      //  printf("Unable to send back data.\n\r");  
      break;  
      
    case 0xA4:
      this->tableUpdate();
           
    case 0xA5:
      //printf("Received check memory command.\n\r");
      //tx_buf[1] = 0xA5;
      //tx_buf[2] = 0x00;
      //tx_buf[3] = 0x04;
      //tx_buf[4] = 0xFE; // 510
      //tx_buf[5] = 0x01;
      //if(!radio.write(&tx_buf[0], 6)) 
      //  printf("Unable to send back data.\n\r");  
      break;  
      
    case 0xA6:
      //printf("Received clear memory command.\n\r");
      //if(!radio.write(&tx_buf[0], 1)) 
      //  printf("Unable to send back data.\n\r");  
      break; 
  }

  // Resume listening so we catch the next command.
  _radio->startListening();
  return replyResult;
}

// Send standard reply with radio
int LinkCommand::replyOk() {
  // Send back system state (if this function is
  // called it means we're Ok)
  byte tx_buf = 0xAF;
  if(!_radio->write(&tx_buf, 1))   {
    printf("Unable to send back data.\n\r");
    return -1;
  }
}

// Process a table update command
int LinkCommand::dumpLogging(void) {
  // Initialize transmit buffer with:
  // [0] the current state of the Arduino
  // [1] the dump logging command code
  // [2] to be set to the update result
  byte tx_buf[9] = {0xAF,0xA3,0,0,0,0,0,0,0};
  
    
  
}

// Process a table update command
int LinkCommand::tableUpdate(void) {
  int update_result;
  // Initialize the message buffers 
  byte rx_buf[7] = {0,0,0,0,0,0,0};
  // Initialize transmit buffer with:
  // [0] the current state of the Arduino
  // [1] the table update command code
  // [2] to be set to the update result
  byte tx_buf[3] = {0xAF,0xA4,0};
  
  // Read the whole buffer sent with first command:
  // [0] the table update command code (still in radio FIFO)
  // [1] remaining table entries including this one
  // [2] if user is authorized (1) or not (0)
  // [3-6] user tag id
  _radio->read(&rx_buf[0], 7);
  
  int table_count = rx_buf[1];
  for (int i = 1; i < table_count; i++) {
    // Flush buffers and switch to TX mode
    _radio->stopListening();
    
    // Update user entry in table
    update_result = _table->setUserAuth(&rx_buf[3], rx_buf[2]);
    tx_buf[2] = this->mapTableUpdateResult(update_result);
    
    // Send result
    if(!_radio->write(&tx_buf[0], 3))   {
      printf("Unable to send back data.\n\r");
      return -1;
    }
      
    // Setup and wait for next table entry
    _radio->startListening();
    while(!_radio->available()) {delay(1);}
    _radio->read(&rx_buf[0], 7);
    delay(20);
  }
  // Reply for last entry
  _radio->stopListening();
  // Update last user entry in table
  update_result = _table->setUserAuth(&rx_buf[3], rx_buf[2]);
  tx_buf[2] = this->mapTableUpdateResult(update_result);
  // Send result
  if(!_radio->write(&tx_buf[0], 3))  {
    printf("Unable to send back data.\n\r");
    return -1;
  }
}

// Map the AccessTable::setUserAuth return argument to
// the proper table update command answer
byte LinkCommand::mapTableUpdateResult(int update_result) {
  switch(update_result) {
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

