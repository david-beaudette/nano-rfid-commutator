/*
  AccessTable.h
  
  Defines the list of user ID's and associated authorisations.
 */
#ifndef __ACCESSTABLE__
#define __ACCESSTABLE__

#include "Arduino.h"
#include <EEPROM.h>

#define MAX_USER_SIZE    248
#define MAX_EEPROM_SIZE 1024

const int userStartAddr = 0;
const int authStartAddr = userStartAddr + 4 * MAX_USER_SIZE;
const int userCountAddr = MAX_EEPROM_SIZE - 2;

class AccessTable {
  public:
    unsigned int getNumUsers();
    int setNumUsers(unsigned int numUsers);
    
    int setAuth(int tableIndex, byte auth);
    int setUserAuth(byte *tad_id, byte auth);
    int getUserIndex(byte *tad_id, int tagLen = 4);
    int addUser(byte *tad_id, byte auth);
    int clearTable();
    
  private:
};


#endif // __ACCESSTABLE__
