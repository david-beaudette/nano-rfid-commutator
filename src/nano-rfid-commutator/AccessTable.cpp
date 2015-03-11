/*
  AccessTable.cpp
  
  Defines the list of user ID's and associated authorisations.
 */

#include "AccessTable.h"

unsigned int AccessTable::getNumUsers() {
  unsigned int countLSB = EEPROM.read(userCountAddr+0);
  unsigned int countMSB = EEPROM.read(userCountAddr+1);
  return countLSB + (countMSB << 8);
};

int AccessTable::setNumUsers(unsigned int numUsers) {
  if(numUsers > MAX_USER_SIZE) {
    return -1;
  }
  EEPROM.write(userCountAddr+0, numUsers & 0xFF);
  EEPROM.write(userCountAddr+1, (numUsers >> 8) & 0xFF);
  return 0;
};

int AccessTable::clearTable() {
  // Write a 0 to all bytes of the EEPROM
  for (int i = 0; i < MAX_EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  return 0;
}
int AccessTable::setAuth(int tableIndex, byte auth) {
  byte tableByte = EEPROM.read(authStartAddr + tableIndex/8);
  byte byteMask  = 1 << (tableIndex%8);
  byte curAuth   = tableByte & byteMask;
  if(curAuth > 0) {
    // User authorized
    if(auth == curAuth) {
      // No change
      return 0;
    }
    else {
      // Change authorization (bitwise xor used)
      tableByte = tableByte ^ byteMask;
      EEPROM.write(authStartAddr + tableIndex/8, tableByte);
      return 1;
    }
  } 
}

int AccessTable::setUserAuth(byte *tag_id, byte auth) {
  // Check for user tag in table
  int tableIndex = this->getUserIndex(tag_id);
  
  if(tableIndex >= 0) {
    return this->setAuth(tableIndex, auth);
  }
  else {
    // Add user function returns 
    //  0 if successful, 
    // -1 if table is full
    return (this->addUser(tag_id, auth) - 1);
  }
}

int AccessTable::getUserIndex(byte *tag_id, int tagLen) {
  int byteNum;
  int curUser;
  int tableIndex = -1;
  
  // Find this tag in EEPROM table
  for(curUser = 0; curUser < this->getNumUsers(); curUser++) {
    // Serial.print("Comparing key #");
    // Serial.print(curUser);
    // Serial.print(" with serial number ");
    // for (i = 0; i < 3; i++) {
    //   Serial.print(pgm_read_byte_near(user_start_addr + curUser*4 + i), HEX);
    //   Serial.print(", ");
    // }
    // Serial.println(pgm_read_byte_near(user_start_addr + curUser*4 + 3), HEX);
    byteNum = 0;
    while(EEPROM.read(userStartAddr + curUser*4 + byteNum) == tag_id[byteNum] && \
          byteNum < 4) {
       byteNum++;
    }
    if(byteNum == 4) {
      // All bytes compared equal
      tableIndex = curUser;
      break;
    }
  }     
  return tableIndex;
}

int AccessTable::addUser(byte *tad_id, byte auth) {
  int numUsers = this->getNumUsers();
  if(numUsers >= MAX_USER_SIZE) {
    // Table full
    return -1;
  }
  // Write authorization bit
  byte tableByte = EEPROM.read(authStartAddr + numUsers/8);
  byte byteMask  = 1 << (numUsers%8);
  byte curAuth   = tableByte & byteMask;
  EEPROM.write(authStartAddr + numUsers/8, tableByte);
  
  // Write user tag
  for(int byteNum = 0; byteNum < 4; byteNum++) {
    EEPROM.write(userStartAddr + numUsers*4 + byteNum, tad_id[byteNum]);
  }
  // Increase table size
  this->setNumUsers(numUsers + 1);  
  
  return 0;
}
