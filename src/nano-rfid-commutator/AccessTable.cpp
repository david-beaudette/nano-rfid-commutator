/*
  AccessTable.cpp
  
  Defines the list of user ID's and associated authorisations.
 */

#include "AccessTable.h"

AccessTable::AccessTable() {
  
}

AccessTable::~AccessTable() {

}

AccessTable::clearTable() {
  // Write a 0 to all bytes of the EEPROM
  for (int i = 0; i < MAX_EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
}

