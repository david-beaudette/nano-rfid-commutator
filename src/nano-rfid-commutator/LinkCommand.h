/*
  LinkCommand.h
  
  Manages commands received in the radio.
 */
#ifndef __LINKCOMMAND__
#define __LINKCOMMAND__

#include <EEPROM.h>
#include "Arduino.h"

#include <RF24.h>
#include "AccessEvent.h"
#include "AccessTable.h"

// Radio configuration
const uint64_t pipes[2]    = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
const uint8_t radioChannel = 0x4c;
const uint8_t payloadSize  = 10;
const int retryDelay       = 15;
const int retryCount       = 15;
const rf24_datarate_e dataRate = RF24_1MBPS;

class LinkCommand {
  public:
    LinkCommand(RF24 *radio, 
                AccessTable *table, 
                EventList *event_list);
    int processCommand(int *systemState);
    
  private:
    int replyOk(void);
    int dumpLogging(void);
    int tableUpdate(void);
    byte mapTableUpdateResult(int retArg);
    RF24 *_radio; 
    AccessTable *_table; 
    EventList *_event_list;
};


#endif // __LINKCOMMAND__
