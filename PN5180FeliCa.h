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
#ifndef PN5180FeliCa_H
#define PN5180FeliCa_H

#include "PN5180.h"

class PN5180FeliCa : public PN5180 {

public:
  PN5180FeliCa(uint8_t SSpin, uint8_t BUSYpin, uint8_t RSTpin);

public:
  uint8_t pol_req(uint8_t *buffer);
  /*
   * Helper functions
   */
public:   
  bool setupRF();
  uint8_t readCardSerial(uint8_t *buffer);    
  bool isCardPresent();    
};

#endif /* PN5180FeliCa_H */
