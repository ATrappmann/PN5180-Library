// NAME: Debug.cpp
//
// DESC: Helper functions for debugging
//
<<<<<<< HEAD
#include <inttypes.h>
=======

#include <inttypes.h>
#include <Wstring.h>
>>>>>>> 3dd96694226e6e15600c7a160eeb6c93e834ca98
#include "Debug.h"

static const char hexChar[] = "0123456789ABCDEF";
static char hexBuffer[9];

char * formatHex(const uint8_t val) {
  hexBuffer[0] = hexChar[val >> 4];
  hexBuffer[1] = hexChar[val & 0x0f];
  hexBuffer[2] = '\0';
  return hexBuffer;
}

char * formatHex(const uint16_t val) {
  hexBuffer[0] = hexChar[val >> 12];
  hexBuffer[1] = hexChar[val >> 8];
  hexBuffer[2] = hexChar[val >> 4];
  hexBuffer[3] = hexChar[val & 0x000f];
  hexBuffer[4] = '\0';
  return hexBuffer;
}

char * formatHex(const uint32_t val) {
  hexBuffer[0] = hexChar[val >> 28];
  hexBuffer[1] = hexChar[val >> 24];
  hexBuffer[2] = hexChar[val >> 20];
  hexBuffer[3] = hexChar[val >> 16];
  hexBuffer[4] = hexChar[val >> 12];
  hexBuffer[5] = hexChar[val >> 8];
  hexBuffer[6] = hexChar[val >> 4];
  hexBuffer[7] = hexChar[val & 0x0f];
  hexBuffer[8] = '\0';
  return hexBuffer;
}
