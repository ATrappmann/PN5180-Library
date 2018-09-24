// NAME: PN5180ISO15693.h
//
// DESC: ISO15693 protocol on NXP Semiconductors PN5180 module for Arduino.
//
#ifndef PN5180ISO15693_H
#define PN5180ISO15693_H

#include "PN5180.h"

class PN5180ISO15693 {
private:
  PN5180  *nfc;

public:
  PN5180ISO15693(PN5180 *nfc);

private:
  bool issueISO15693Command(uint8_t *cmd, uint8_t cmdLen, uint8_t **resultPtr);

public:   
  bool getInventory(uint8_t *uid);
  bool readSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *readBuffer, uint8_t blockSize);
  bool getSystemInfo(uint8_t *uid, uint8_t *blockSize, uint8_t *numBlocks);

  bool setupRF();
  
};

#endif /* PN5180ISO15693_H */
