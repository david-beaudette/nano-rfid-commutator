/*
  AccessEvent.cpp
  
  Defines the objects used to log access events on the device.

*/

#include "AccessEvent.h"

EventList::EventList(Timer *t) {
  // Reset FIFO and counter
  _list_start = 0;
  _list_stop  = 0; 
  _isempty    = 1;  
  event_list_counter = 0UL;
  
  // Save timer instance
  _t = t;
  
}


AccessEvent* EventList::getEvent(void) {
  if(_isempty) return NULL;
  // Retrieve the oldest event 
  AccessEvent *oldest = &_list[_list_start];
  
  // The desired time is the number of seconds since the 
  // event happened. This is computed from the current counter
  // value minus the counter value when the event happened. 
  _list[_list_start].time = event_list_counter - _list[_list_start].time;
  
  // Remove event from FIFO
  _list_start += 1;  
  if(_list_start >= MAX_NUM_EVENTS) {
    // Wrap around
    _list_start = 0;
  }
  
  // If this was the last recorded event, reset counter
  if(_list_start == _list_stop) {
    event_list_counter  = 0UL;
    _isempty = 1;  
  }
  
  // Return event 
  return oldest;
}

int EventList::addEvent(byte type, byte *tag_id) {
  // Check FIFO state
  if(getListSize() == MAX_NUM_EVENTS) {
    // FIFO full
    return -1;
  }
  
  // Assign event information
  _list[_list_stop].type = type;
  _list[_list_stop].time = event_list_counter;
  memcpy(&_list[_list_stop].tag_id[0], tag_id, 4 * sizeof(byte));
  
  // Add event to FIFO
  _list_stop += 1;
  if(_list_stop >= MAX_NUM_EVENTS) {
    // Wrap around
    _list_stop = 0;
  }
  if(_isempty) {
    // Reset counter
    event_list_counter = 0UL;
    _isempty = 0;
  }
  
  return 0;
}

int EventList::getListSize() {
  if(_isempty) return 0;
  if(_list_stop > _list_start) {
    return (_list_stop - _list_start);
  }
  else {
    return (_list_stop + (MAX_NUM_EVENTS - _list_start));
  }
}

