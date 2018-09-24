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
extern String formatHex(const uint8_t val);
extern String formatHex(const uint16_t val);
extern String formatHex(const uint32_t val);
#endif

#endif /* DEBUG_H */
