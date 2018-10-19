// NAME: Debug.h
//
// DESC: Helper methods for debugging
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
