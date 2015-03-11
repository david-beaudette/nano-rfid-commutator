/*
  LinkCommand.cpp
  
  Defines the list of user ID's and associated authorisations.
 */

#include "LinkCommand.h"

void LinkCommand::LinkCommand(RF24 *radio, 
                              AccessTable *table, 
                              AccessEvent *events) {
  _radio = radio;
  _table = table;
  _events = events;  

}
