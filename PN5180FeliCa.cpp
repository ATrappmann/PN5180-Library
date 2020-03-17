// NAME: PN5180FeliCa.h
//
// DESC: FeliCa protocol on NXP Semiconductors PN5180 module for Arduino.
//
// Copyright (c) 2020 by CrazyRedMachine. All rights reserved.
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
#include "PN5180FeliCa.h"
#include <PN5180.h>
#include "Debug.h"

PN5180FeliCa::PN5180FeliCa(uint8_t SSpin, uint8_t BUSYpin, uint8_t RSTpin)
              : PN5180(SSpin, BUSYpin, RSTpin) {
}

bool PN5180FeliCa::setupRF() {
  PN5180DEBUG(F("Loading RF-Configuration...\n"));
  if (loadRFConfig(0x09, 0x89)) {  // FeliCa 424 parameters
    PN5180DEBUG(F("done.\n"));
  }
  else return false;

  PN5180DEBUG(F("Turning ON RF field...\n"));
  if (setRF_on()) {
    PN5180DEBUG(F("done.\n"));
  }
  else return false;

  return true;
}

/*
* buffer : must be 20 byte array
* buffer[0-1] is length and 01
* buffer[2..9] is IDm
* buffer[10..17] is PMm.
* buffer[18..19] is POL_RES data
*
* return value: the uid length in bytes:
* -	zero if no tag was recognized
* -	8 if a FeliCa tag was recognized
*/
uint8_t PN5180FeliCa::pol_req(uint8_t *buffer) {
	uint8_t cmd[6];
	uint8_t uidLength = 0;
	// Load FeliCa 424 protocol
	if (!loadRFConfig(0x09, 0x89))
	  return 0;
	// OFF Crypto
	if (!writeRegisterWithAndMask(SYSTEM_CONFIG, 0xFFFFFFBF))
	  return 0;

	//send FeliCa request (every packet starts with length)
    cmd[0] = 0x06;             //total length
	cmd[1] = 0x00;             //POL_REQ command
	cmd[2] = 0xFF;             //
	cmd[3] = 0xFF;             // any target
	cmd[4] = 0x01;             // System Code request
	cmd[5] = 0x00;             // 1 timeslot only

	if (!sendData(cmd, 6, 0x00))
	  return 0;
    //wait a little to avoid bug with some cards
    delay(50);

    //response packet should be 0x14 (20 bytes total length), 0x01 Response Code, 8 IDm bytes, 8 PMm bytes, 2 Request Data bytes
    //READ 20 bytes reply
  uint8_t *internalBuffer = readData(20);
	if (!internalBuffer)
	  return 0;
 for (int i=0; i<20; i++) {
    buffer[i] = internalBuffer[i];
 }
 
    //check Response Code
    if ( buffer[1] != 0x01 ){
        uidLength = 0;
    } else {
        uidLength = 8;
    }
    return uidLength;
}

uint8_t PN5180FeliCa::readCardSerial(uint8_t *buffer) {

    uint8_t response[20];
	uint8_t uidLength;

    for (int i = 0; i < 20; i++)
        response[i] = 0;

    uidLength = pol_req(response);

    // no reply
	if (uidLength == 0)
	  return 0;

    for (int i = 0; i < uidLength; i++)
        buffer[i] = response[i+2];

	return uidLength;
}

bool PN5180FeliCa::isCardPresent() {
    uint8_t buffer[8];
	return (readCardSerial(buffer) != 0);
}
