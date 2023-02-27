// Clock Module Initialization
//
// November 16, 2022
// Copyright 2022, Thomas C. McDermott, N5EG
// Licensed under the GNU Public License, GPL, Version 2 or later.

// This contains a text file with the address+data records for initializating
// the Si5345 synthesizer chip.

#ifndef __CLOCKMODINIT_H__
#define __CLOCKMODINIT_H__


#include "system.h"
#include "alt_error_handler.h"

#define CLOCKPREFACECOUNT	4		// These may need to be increased if the Clockbuilder file
#define CLOCKBODYCOUNT		530		// generates more records.

struct ClockRecord
{
	alt_u16 address;
	alt_u8 data;
};

int ParseClockRec(struct ClockRecord *, alt_u8 *, alt_u8 *, alt_u8 *); // Read clock record and parse

struct ClockRecord ClockPreface[CLOCKPREFACECOUNT];

struct ClockRecord ClockBody[CLOCKBODYCOUNT];

#endif /* __CLOCKMODINIT_H__ */
