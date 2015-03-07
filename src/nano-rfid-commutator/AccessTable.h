/*
  AccessTable.h
  
  Defines the list of user ID's and associated authorisations.
 */
#include <EEPROM.h>

#define MAX_USER_SIZE    248
#define MAX_EEPROM_SIZE 1024
 
#ifndef __ACCESSTABLE__
#define __ACCESSTABLE__

class AccessTable {
  public:
    int getNumUsers() {return _numUsers};
    int getAuth(byte *tad_id);
    int addUser(byte *tad_id, int auth);
    int clearTable();
  private:
    int _numUsers;

}


#endif // __ACCESSTABLE__
