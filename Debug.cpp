// NAME: Debug.cpp
// 
// DESC: Helper functions for debugging
//

#include <Arduino.h>
#include "Debug.h"

String formatHex(const uint8_t val) {
  const char hexChar[] = "0123456789ABCDEF";
  
  uint8_t hi = (val & 0xf0) >> 4;
  uint8_t lo = (val & 0x0f);
  
  String s = ""; 
  s += hexChar[hi];
  s += hexChar[lo];

  return s;
}

String formatHex(const uint16_t val) {
  uint8_t hi = (val & 0xff00) >> 8;
  uint8_t lo = (val & 0x00ff);

  String s = "";
  s += formatHex(hi);
  s += formatHex(lo);

  return s;
}

String formatHex(const uint32_t val) {
  uint16_t hi = (val & 0xffff0000) >> 16;
  uint16_t lo = (val & 0x0000ffff);

  String s = "";
  s += formatHex(hi);
  s += formatHex(lo);

  return s;
}
