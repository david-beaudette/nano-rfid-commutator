/*
  AccessEvent.h
  
  Defines the object used to log access events on the device.
 */

#ifndef __ACCESSEVENT__
#define __ACCESSEVENT__

class AccessEvent {
  public:
    byte type;
    unsigned long time;
    byte tag_id[4];
}


#endif // __ACCESSEVENT__
