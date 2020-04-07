// NAME: PN5180iClass.cpp
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
//#define DEBUG 1

#include <Arduino.h>
#include "PN5180iClass.h"
#include "Debug.h"

PN5180iClass::PN5180iClass(uint8_t SSpin, uint8_t BUSYpin, uint8_t RSTpin) : PN5180(SSpin, BUSYpin, RSTpin) {
}

iClassErrorCode PN5180iClass::ActivateAll() {
  PN5180DEBUG(F("Activate All...\n"));

  // Disable CRCs
  writeRegister(CRC_TX_CONFIG, 0x00000000);
  writeRegister(CRC_RX_CONFIG, 0x00000000);

  uint8_t actall[] = {ICLASS_CMD_ACTALL};

  uint8_t *readBuffer;
  iClassErrorCode rc = issueiClassCommand(actall, sizeof(actall), &readBuffer);
  if (ICLASS_EC_OK != rc) {
    return rc;
  }

  return ICLASS_EC_OK;
}

iClassErrorCode PN5180iClass::Identify(uint8_t *csn) {
  PN5180DEBUG(F("Identify...\n"));

  writeRegister(CRC_TX_CONFIG, 0x00000000);
  writeRegister(CRC_RX_CONFIG, 0x00000029);

  uint8_t identify[] = {ICLASS_CMD_IDENTIFY};

  for (int i=0; i<8; i++) {
    csn[i] = 0;
  }

  uint8_t *readBuffer;
  iClassErrorCode rc = issueiClassCommand(identify, sizeof(identify), &readBuffer);
  if (ICLASS_EC_OK != rc) {
    return rc;
  }

  // Anticollision CSN
  for (int i=0; i<8; i++) {
    csn[i] = readBuffer[i];
  }

  return ICLASS_EC_OK;
}


iClassErrorCode PN5180iClass::Select(uint8_t *csn) {
  PN5180DEBUG(F("Select...\n"));

  writeRegister(CRC_TX_CONFIG, 0x00000000);
  writeRegister(CRC_RX_CONFIG, 0x00000029);

  uint8_t select[] = {ICLASS_CMD_SELECT, 1, 2, 3, 4, 5, 6, 7, 8};

  for (int i=0; i<8; i++) {
    select[i+1] = csn[i];
  }

  uint8_t *readBuffer;
  iClassErrorCode rc = issueiClassCommand(select, sizeof(select), &readBuffer);
  if (ICLASS_EC_OK != rc) {
    return rc;
  }

  // Replace with real CSN
  for (int i=0; i<8; i++) {
    csn[i] = readBuffer[i];
  }

  return ICLASS_EC_OK;
}

iClassErrorCode PN5180iClass::ReadCheck(uint8_t *ccnr) {
  PN5180DEBUG(F("ReadCheck...\n"));

  // Disable CRCs
  writeRegister(CRC_TX_CONFIG, 0x00000000);
  writeRegister(CRC_RX_CONFIG, 0x00000000);

  uint8_t readcheck[] = {ICLASS_CMD_READCHECK, 0x02};

  uint8_t *readBuffer;
  iClassErrorCode rc = issueiClassCommand(readcheck, sizeof(readcheck), &readBuffer);
  if (ICLASS_EC_OK != rc) {
    return rc;
  }

  for (int i=0; i<8; i++) {
    ccnr[i] = readBuffer[i];
  }

  return ICLASS_EC_OK;
}

iClassErrorCode PN5180iClass::Check(uint8_t *mac) {
  PN5180DEBUG(F("Check...\n"));

  // Disable CRCs
  writeRegister(CRC_TX_CONFIG, 0x00000000);
  writeRegister(CRC_RX_CONFIG, 0x00000000);

  uint8_t check[] = {ICLASS_CMD_CHECK, 0, 0, 0, 0, 1, 2, 3, 4};

  // Copied into last 4 bytes
  for (int i=0; i<4; i++) {
    check[i+5] = mac[i];
  }

  uint8_t *readBuffer;
  iClassErrorCode rc = issueiClassCommand(check, sizeof(check), &readBuffer);
  if (ICLASS_EC_OK != rc) {
    return rc;
  }

  return ICLASS_EC_OK;
}

iClassErrorCode PN5180iClass::Read(uint8_t blockNum, uint8_t *blockData) {
  PN5180DEBUG(F("Read...\n"));

  writeRegister(CRC_TX_CONFIG, 0x00000069);
  writeRegister(CRC_RX_CONFIG, 0x00000029);

  uint8_t read[] = {ICLASS_CMD_READ, blockNum};

  uint8_t *readBuffer;
  iClassErrorCode rc = issueiClassCommand(read, sizeof(read), &readBuffer);
  if (ICLASS_EC_OK != rc) {
    return rc;
  }

  for (int i=0; i<8; i++) {
    blockData[i] = readBuffer[i];
  }

  return ICLASS_EC_OK;
}

iClassErrorCode PN5180iClass::Halt() {
  PN5180DEBUG(F("Halt...\n"));

  // Disable CRCs
  writeRegister(CRC_TX_CONFIG, 0x00000000);
  writeRegister(CRC_RX_CONFIG, 0x00000000);

  uint8_t halt[] = {ICLASS_CMD_HALT};

  uint8_t *readBuffer;
  iClassErrorCode rc = issueiClassCommand(halt, sizeof(halt), &readBuffer);
  if (ICLASS_EC_OK != rc) {
    return rc;
  }

  return ICLASS_EC_OK;
}

iClassErrorCode PN5180iClass::issueiClassCommand(uint8_t *cmd, uint8_t cmdLen, uint8_t **resultPtr) {
#ifdef DEBUG
  PN5180DEBUG(F("Issue Command 0x"));
  PN5180DEBUG(formatHex(cmd[1]));
  PN5180DEBUG("...\n");
#endif

  sendData(cmd, cmdLen);
  delay(10);

  if (0 == (getIRQStatus() & RX_SOF_DET_IRQ_STAT)) {
    return EC_NO_CARD;
  }

  uint32_t rxStatus;
  readRegister(RX_STATUS, &rxStatus);

  PN5180DEBUG(F("RX-Status="));
  PN5180DEBUG(formatHex(rxStatus));

  uint16_t len = (uint16_t)(rxStatus & 0x000001ff);

  PN5180DEBUG(", len=");
  PN5180DEBUG(len);
  PN5180DEBUG("\n");

 *resultPtr = readData(len);
  if (0L == *resultPtr) {
    PN5180DEBUG(F("*** ERROR in readData!\n"));
    return ICLASS_EC_UNKNOWN_ERROR;
  }

#ifdef DEBUG
  Serial.print("Read=");
  for (int i=0; i<len; i++) {
    Serial.print(formatHex((*resultPtr)[i]));
    if (i<len-1) Serial.print(":");
  }
  Serial.println();
#endif

  uint32_t irqStatus = getIRQStatus();
  if (0 == (RX_SOF_DET_IRQ_STAT & irqStatus)) { // no card detected
     clearIRQStatus(TX_IRQ_STAT | IDLE_IRQ_STAT);
     return EC_NO_CARD;
  }

  // Datasheet Picopass 2K V1.0  section 4.3.2
  if (RX_SOF_DET_IRQ_STAT == (RX_SOF_DET_IRQ_STAT & irqStatus)) {
    clearIRQStatus(RX_SOF_DET_IRQ_STAT);
    return ICLASS_EC_OK;
  }

  uint8_t responseFlags = (*resultPtr)[0];
  if (responseFlags & (1<<0)) { // error flag
    uint8_t errorCode = (*resultPtr)[1];

    PN5180DEBUG("ERROR code=");
    PN5180DEBUG(formatHex(errorCode));
    PN5180DEBUG(" - ");
    PN5180DEBUG(strerror(errorCode));
    PN5180DEBUG("\n");

    return (iClassErrorCode)errorCode;
  }

#ifdef DEBUG
  if (responseFlags & (1<<3)) { // extendsion flag
    PN5180DEBUG("Extension flag is set!\n");
  }
#endif

  clearIRQStatus(RX_SOF_DET_IRQ_STAT | IDLE_IRQ_STAT | TX_IRQ_STAT | RX_IRQ_STAT);
  return ICLASS_EC_OK;
}

bool PN5180iClass::setupRF() {
  PN5180DEBUG(F("Loading RF-Configuration...\n"));
  if (loadRFConfig(0x0d, 0x8d)) {  // ISO15693 parameters
    PN5180DEBUG(F("done.\n"));
  }
  else return false;

  PN5180DEBUG(F("Turning ON RF field...\n"));
  if (setRF_on()) {
    PN5180DEBUG(F("done.\n"));
  }
  else return false;

  writeRegisterWithAndMask(SYSTEM_CONFIG, 0xfffffff8);  // Idle/StopCom Command
  writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000003);   // Transceive Command

  return true;
}

const __FlashStringHelper *PN5180iClass::strerror(iClassErrorCode errno) {
  PN5180DEBUG(F("iClassErrorCode="));
  PN5180DEBUG(errno);
  PN5180DEBUG("\n");

  switch (errno) {
    case EC_NO_CARD: return F("No card detected!");
    case ICLASS_EC_OK: return F("OK!");
    default:
      return F("Undefined error code in iClass!");
  }
}
