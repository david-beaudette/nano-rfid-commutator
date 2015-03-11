/*
  LinkCommand.h
  
  Manages commands received in the radio.
 */
#ifndef __LINKCOMMAND__
#define __LINKCOMMAND__

#include <EEPROM.h>
#include "Arduino.h"



#define MAX_USER_SIZE    
#define MAX_EEPROM_SIZE 1

const int userStartAddr =
const int authStartAddr =
const int userCountAddr =

class LinkCommand {
  public:
    void LinkCommand(RF24 *radio, 
                     AccessTable *table, 
                     AccessEvent *events);
    
  private:
    RF24 *_radio; 
    AccessTable *_table; 
    AccessEvent *_events;
};


#endif // __LINKCOMMAND__
