// NAME: PN5180iClass.h
//
// DESC: iClass protocol on NXP Semiconductors PN5180 module for Arduino.
//
// Copyright (c) 2018 by Andreas Trappmann. All rights reserved.
//
// This file is part of the PN5180 library for the Arduino environment.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
#ifndef PN5180ICLASS_H
#define PN5180ICLASS_H

#include "PN5180.h"

enum {
  ICLASS_CMD_HALT = 0x00,
  ICLASS_CMD_ACTALL = 0x0A,
  ICLASS_CMD_IDENTIFY = 0x0C,
  ICLASS_CMD_SELECT = 0x81,
  ICLASS_CMD_READCHECK = 0x88,
  ICLASS_CMD_CHECK = 0x05,
  ICLASS_CMD_READ = 0x0C,
};

enum iClassErrorCode {
  EC_NO_CARD = -1,
  ICLASS_EC_OK = 0,
  ICLASS_EC_UNKNOWN_ERROR = 0xFE,
};

class PN5180iClass : public PN5180 {

public:
  PN5180iClass(uint8_t SSpin, uint8_t BUSYpin, uint8_t RSTpin);

private:
  iClassErrorCode issueiClassCommand(uint8_t *cmd, uint8_t cmdLen, uint8_t **resultPtr);
public:
  iClassErrorCode ActivateAll();
  iClassErrorCode Identify(uint8_t *csn);
  iClassErrorCode Select(uint8_t *csn);
  iClassErrorCode ReadCheck(uint8_t *ccnr);
  iClassErrorCode Check(uint8_t *mac);
  iClassErrorCode Read(uint8_t blockNum, uint8_t *blockData);
  iClassErrorCode Halt();

  /*
   * Helper functions
   */
public:
  bool setupRF();
  const __FlashStringHelper *strerror(iClassErrorCode errno);

};

#endif /* PN5180ICLASS_H */
