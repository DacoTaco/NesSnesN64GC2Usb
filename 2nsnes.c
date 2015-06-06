/* Name: fournsnes.c
 * Project: Multiple NES/SNES to USB converter
 * Author: Raphael Assenat <raph@raphnet.net>
 * Copyright: (C) 2007-2012 Raphael Assenat <raph@raphnet.net>
 * License: GPLv2
 * Tabsize: 4
 */
 
 //#define SNES_MODE
 
 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "gamepad.h"
#include "2nsnes.h"

#ifndef SNES_MODE
#include "reportdesc.h"
#endif

#define GAMEPAD_BYTES	4	/* 2 byte per snes controller * 4 controllers */

/******** IO port definitions **************/
#define SNES_LATCH_DDR	DDRC
#define SNES_LATCH_PORT	PORTC
#define SNES_LATCH_BIT	(1<<1)

#define SNES_CLOCK_DDR	DDRC
#define SNES_CLOCK_PORT	PORTC
#define SNES_CLOCK_BIT	(1<<2)

#define SNES_DATA_PORT	PORTC
#define SNES_DATA_DDR	DDRC
#define SNES_DATA_PIN	PINC
#define SNES_DATA_BIT1	(1<<3)	/* controller 1 */
#define SNES_DATA_BIT2	(1<<0)	/* controller 2 */

/********* IO port manipulation macros **********/
#define SNES_LATCH_LOW()	do { SNES_LATCH_PORT &= ~(SNES_LATCH_BIT); } while(0)
#define SNES_LATCH_HIGH()	do { SNES_LATCH_PORT |= SNES_LATCH_BIT; } while(0)
#define SNES_CLOCK_LOW()	do { SNES_CLOCK_PORT &= ~(SNES_CLOCK_BIT); } while(0)
#define SNES_CLOCK_HIGH()	do { SNES_CLOCK_PORT |= SNES_CLOCK_BIT; } while(0)

#define SNES_GET_DATA1()	(SNES_DATA_PIN & SNES_DATA_BIT1)
#define SNES_GET_DATA2()	(SNES_DATA_PIN & SNES_DATA_BIT2)

/*********** prototypes *************/
static void fournsnesInit(void);
static char fournsnesUpdate(void);
static char fournsnesChanged(unsigned char report_id);
static char fournsnesBuildReport(unsigned char *reportBuffer, char report_id);
static void setupDescriptors();
Gamepad SnesGamepad;

// the most recent bytes we fetched from the controller
static unsigned char last_read_controller_bytes[GAMEPAD_BYTES];

// the most recently reported bytes
static unsigned char last_reported_controller_bytes[GAMEPAD_BYTES];

// indicates if a controller is in NES mode
static unsigned char nesMode=0;	/* Bit0: controller 1, Bit1: controller 2...*/
//static unsigned char live_autodetect = 1;

/*void disableLiveAutodetect(void)
{
	live_autodetect = 0;
}*/

static void fournsnesInit(void)
{
	unsigned char sreg;
	sreg = SREG;
	cli();
	
	// clock and latch as output
	SNES_LATCH_DDR |= SNES_LATCH_BIT;
	SNES_CLOCK_DDR |= SNES_CLOCK_BIT;

	// data as input
	SNES_DATA_DDR &= ~(SNES_DATA_BIT1 | SNES_DATA_BIT2);
	// enable pullup. This should prevent random toggling of pins
	// when no controller is connected.
	SNES_DATA_PORT |= (SNES_DATA_BIT1 | SNES_DATA_BIT2);

	// clock is normally high
	SNES_CLOCK_PORT |= SNES_CLOCK_BIT;

	// LATCH is Active HIGH
	SNES_LATCH_PORT &= ~(SNES_LATCH_BIT);
	

	nesMode = 0;
	fournsnesUpdate();

	/*if (!live_autodetect) {	
		/* Snes controller buttons are sent in this order:
		 * 1st byte: B Y SEL START UP DOWN LEFT RIGHT 
		 * 2nd byte: A X L R 1 1 1 1
		 *
		 * Nes controller buttons are sent in this order:
		 * One byte: A B SEL START UP DOWN LEFT RIGHT
		 *
		 * When an additional byte is read from a NES controller,
		 * all bits are 0. Because the data signal is active low,
		 * this corresponds to pressed buttons. When we read
		 * from the controller for the first time, detect NES
		 * controllers by checking those 4 bits.
		 **//*
		if (last_read_controller_bytes[1]==0xFF)
			nesMode |= 1;
		if (last_read_controller_bytes[3]==0xFF)
			nesMode |= 2;
		
	}*/
	setupDescriptors();

	SREG = sreg;
}
static char fournsnesUpdate(void)
{
	int i;
	unsigned char tmp1=0;
	unsigned char tmp2=0;
	/*unsigned char tmp3=0;
	unsigned char tmp4=0;*/
	{			
		SNES_LATCH_HIGH();
		_delay_us(12);
		SNES_LATCH_LOW();

		for (i=0; i<8; i++)
		{
			_delay_us(6);
			SNES_CLOCK_LOW();
			
			tmp1 <<= 1;	
			tmp2 <<= 1;	
			/*tmp3 <<= 1;	
			tmp4 <<= 1;*/
			if (!SNES_GET_DATA1()) { tmp1 |= 1; }
			if (!SNES_GET_DATA2()) { tmp2 |= 1; }

			_delay_us(6);
			SNES_CLOCK_HIGH();
		}
		last_read_controller_bytes[0] = tmp1;
		last_read_controller_bytes[2] = tmp2;
		/*last_read_controller_bytes[4] = tmp3;
		last_read_controller_bytes[6] = tmp4;*/

		for (i=0; i<8; i++)
		{
			_delay_us(6);

			SNES_CLOCK_LOW();

			// notice that this is different from above. We
			// want the bits to be in reverse-order
			tmp1 >>= 1;	
			tmp2 >>= 1;	
			/*tmp3 >>= 1;	
			tmp4 >>= 1;	*/
			if (!SNES_GET_DATA1()) { tmp1 |= 0x80; }
			if (!SNES_GET_DATA2()) { tmp2 |= 0x80; }
			
			_delay_us(6);
			SNES_CLOCK_HIGH();
		}
	}


	//if (live_autodetect) {	
		if (tmp1==0xFF)
			nesMode |= 1;
		else
			nesMode &= ~1;

		if (tmp2==0xFF)
			nesMode |= 2;
		else
			nesMode &= ~2;

		/*if (tmp3==0xFF)
			nesMode |= 4;
		else
			nesMode &= ~4;

		if (tmp4==0xFF)
			nesMode |= 8;
		else
			nesMode &= ~8;*/

	//}

	/* Force extra bits to 0 when in NES mode. Otherwise, if
	 * we read zeros on the wire, we will have permanantly 
	 * pressed buttons */
	last_read_controller_bytes[1] = (nesMode & 1) ? 0x00 : tmp1;
	last_read_controller_bytes[3] = (nesMode & 2) ? 0x00 : tmp2;
	/*last_read_controller_bytes[5] = (nesMode & 4) ? 0x00 : tmp3;
	last_read_controller_bytes[7] = (nesMode & 8) ? 0x00 : tmp4;*/
	return 0;
}

static char fournsnesChanged(unsigned char report_id)
{
	//report_id--; // first report is 1

	return memcmp(	&last_read_controller_bytes[/*report_id<<1*/0], 
					&last_reported_controller_bytes[/*report_id<<1*/0], 
					GAMEPAD_BYTES);
}

#ifdef SNES_MODE
static char getX(unsigned char nesByte1)
{
	char x = 128;
	if (nesByte1&0x1) { x = 255; }
	if (nesByte1&0x2) { x = 0; }
	return x;
}

static char getY(unsigned char nesByte1)
{
	char y = 128;
	if (nesByte1&0x4) { y = 255; }
	if (nesByte1&0x8) { y = 0; }
	return y;
}

static unsigned char snesReorderButtons(unsigned char bytes[2])
{
	unsigned char v;

	/* pack the snes button bits, which are on two bytes, in
	 * one single byte. */
	v =	(bytes[0]&0x80)>>7;
	v |= (bytes[0]&0x40)>>5;
	v |= (bytes[0]&0x20)>>3;
	v |= (bytes[0]&0x10)>>1;
	v |= (bytes[1]&0x0f)<<4;

	return v;
}
#endif

static char fournsnesBuildReport(unsigned char *reportBuffer, char id)
{

	/* last_read_controller_bytes[] structure:
	 *
	 * [0] : controller 1, 8 first bits (dpad + start + sel + y|a + b)
	 * [1] : controller 1, 8 snes extra bits (4 lower bits are buttons)
	 *
	 * [2] : controller 2, 8 first bits
	 * [3] : controller 2, 4 extra snes buttons
	 *
	 * [4] : controller 3, 8 first bits
	 * [5] : controller 3, 4 extra snes buttons
	 *
	 * [6] : controller 4, 8 first bits
	 * [7] : controller 4, 4 extra snes buttons
	 *
	 *
	 * last_read_controller_bytes[] structure in FOUR SCORE mode:
	 *
	 *  A B SEL START UP DOWN LEFT RIGHT 
	 *
	 * [0] : NES controller 1 data
	 * [1] : NES controller 2 data
	 * [2] : NES controller 3 data
	 * [3] : NES controller 4 data
	 *
	 */
#ifdef SNES_MODE
	int idx;

	if (id < 0 || id > 4)
		return 0;
	
	idx = id - 1;
	
	if (reportBuffer != NULL)
	{
		reportBuffer[0]=id;
		reportBuffer[1]=getX(last_read_controller_bytes[idx*2]);
		reportBuffer[2]=getY(last_read_controller_bytes[idx*2]);

		if (nesMode & (0x01<<idx))
			reportBuffer[3] = last_read_controller_bytes[idx*2] & 0xf0;
		else
			reportBuffer[3] = snesReorderButtons(&last_read_controller_bytes[idx*2]);
		
	}

	memcpy(&last_reported_controller_bytes[idx*2], 
			&last_read_controller_bytes[idx*2], 
			2);
			
	return 4;
#else
	
	unsigned char btns1 = 0;
	char i = 0;
	unsigned char x = 128;
	unsigned char y = 128;
	for(i = 0;i < 2;i++)
	{
		if((nesMode & (i+1)))
		{
			//NES layout : A B SEL START UP DOWN LEFT RIGHT
			btns1 |= ((last_read_controller_bytes[i*2] & 0x10) >> 4) + ((last_read_controller_bytes[i*2] & 0x20) << 2) 
				  + ((last_read_controller_bytes[i*2] & 0x40) >> 3) + ((last_read_controller_bytes[i*2] & 0x80) >> 3);
		}
		else
		{
			//btns1 = snesReorderButtons(&last_read_controller_bytes[idx*2]);			
			//reorder snes buttons from the stream controller gave us into the GC-style layout
			//snes layout : B Y SEL START A X L R
			//GC layout : Start Y X B A L R Z DU DD DR DL
			btns1 |= ((last_read_controller_bytes[i*2] & 0x10) >> 4) + ((last_read_controller_bytes[i*2] & 0x40) >> 5) + ((last_read_controller_bytes[(i*2)+1]&0x02) << 1) + ((last_read_controller_bytes[i*2] & 0x80) >> 4)
				  +  ((last_read_controller_bytes[(i*2)+1]&0x01) << 4) + ((last_read_controller_bytes[(i*2)+1]&0x04) << 3) + ((last_read_controller_bytes[(i*2)+1]&0x08) << 3) + ((last_read_controller_bytes[i*2] & 0x20) << 2);
		}
		if ( ((last_read_controller_bytes[i*2]) &0x1) ) 
		{ 
			x = x ? 255 : 128;
		}
		if ( ((last_read_controller_bytes[i*2]) &0x2) )
		{ 
			x = (x==255) ?128 : 0;
		}
		if ( ((last_read_controller_bytes[i*2]) &0x4) )
		{ 
			y = y ? 255 : 128;
		}
		if ( ((last_read_controller_bytes[i*2]) &0x8) )
		{ 
			y = (y==255) ? 128 : 0;
		}
		
	}
	if(reportBuffer != NULL)
	{
		//char dir = getX(last_read_controller_bytes[0]);
		reportBuffer[0] = 1; // report ID
		reportBuffer[1] = x;
		reportBuffer[2] = y;
		reportBuffer[3] = 0x7F;
		reportBuffer[4] = 0x7F;
		// Sliders value to decrease as pushed (v2.x behaviour)
		reportBuffer[5] = 0x7F;
		reportBuffer[6] = 0x7F;
		reportBuffer[7] = btns1;
		reportBuffer[8] = 0;
	}
	
	memcpy(&last_reported_controller_bytes[0], 
			&last_read_controller_bytes[0], 
			GAMEPAD_BYTES);
	
	return GCN64_REPORT_SIZE;
#endif
}

#ifdef SNES_MODE
const char one_nsnes_usbHidReportDescriptor[] PROGMEM = {
	/* Controller and report_id 1 */
    0x05, 0x01,			// USAGE_PAGE (Generic Desktop)
    0x09, 0x04,			// USAGE (Joystick)
    0xa1, 0x01,			//	COLLECTION (Application)
    0x09, 0x01,			//		USAGE (Pointer)
    0xa1, 0x00,			//		COLLECTION (Physical)
	0x85, 0x01,			//			REPORT_ID (1)
	0x09, 0x30,			//			USAGE (X)
    0x09, 0x31,			//			USAGE (Y)
    0x15, 0x00,			//			LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,	//			LOGICAL_MAXIMUM (255)
    0x75, 0x08,			//			REPORT_SIZE (8)
    0x95, 0x02,			//			REPORT_COUNT (2)
    0x81, 0x02,			//			INPUT (Data,Var,Abs)

    0x05, 0x09,			//			USAGE_PAGE (Button)
    0x19, 1,			//   		USAGE_MINIMUM (Button 1)
    0x29, 8,			//   		USAGE_MAXIMUM (Button 8)
    0x15, 0x00,			//   		LOGICAL_MINIMUM (0)
    0x25, 0x01,			//   		LOGICAL_MAXIMUM (1)
    0x75, 1,			// 			REPORT_SIZE (1)
    0x95, 8,			//			REPORT_COUNT (8)
    0x81, 0x02,			//			INPUT (Data,Var,Abs)
	0xc0,				//		END_COLLECTION
    0xc0,				// END_COLLECTION
	};

#endif 

static void setupDescriptors()
{
	//mod for only 1 device
#ifdef SNES_MODE
	SnesGamepad.num_reports = 1;
	SnesGamepad.reportDescriptor = (void*)one_nsnes_usbHidReportDescriptor;
	SnesGamepad.reportDescriptorSize = sizeof(one_nsnes_usbHidReportDescriptor);
#else
	SnesGamepad.reportDescriptor = (void*)gcn64_usbHidReportDescriptor;
	SnesGamepad.reportDescriptorSize = getUsbHidReportDescriptor_size();
#endif
	return;
}

Gamepad SnesGamepad = {
	.num_reports			= 1,
#ifdef SNES_MODE
	.reportDescriptorSize 	= sizeof(one_nsnes_usbHidReportDescriptor),
#endif
	.init					= fournsnesInit,
	.update					= fournsnesUpdate,
	.changed				= fournsnesChanged,
	.buildReport			= fournsnesBuildReport
};

Gamepad *fournsnesGetGamepad(void)
{
	return &SnesGamepad;
}

