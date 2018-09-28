//
// Helper methods for debugging
//
#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define PN5180DEBUG(msg) Serial.print(msg)
#else
#define PN5180DEBUG(msg)
#endif

#ifdef DEBUG
extern char * formatHex(const uint8_t val);
extern char * formatHex(const uint16_t val);
extern char * formatHex(const uint32_t val);
#endif

#endif /* DEBUG_H */
