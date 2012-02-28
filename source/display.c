/* ========================================================================
 * [PROJECT]    SIR100
 * [MODULE]     Display
 * [TITLE]      display source file
 * [FILE]       display.c
 * [VSN]        1.0
 * [CREATED]    26092003
 * [LASTCHNGD]  06102006
 * [COPYRIGHT]  Copyright (C) STREAMIT BV
 * [PURPOSE]    contains all interface- and low-level routines to
 *              control the LCD and write characters or strings (menu-items)
 * ======================================================================== */

#define LOG_MODULE  LOG_DISPLAY_MODULE

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/timer.h>
#include <sys/event.h>
#include <sys/thread.h>
#include <sys/heap.h>

#include "system.h"
#include "portio.h"
#include "display.h"
#include "log.h"

/*-------------------------------------------------------------------------*/
/* local defines                                                           */
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* local variable definitions                                              */
/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
/* local routines (prototyping)                                            */
/*-------------------------------------------------------------------------*/
static void LcdWriteByte(u_char, u_char);
static void LcdWriteNibble(u_char, u_char);
static void LcdWaitBusy(void);

/*!
 * \addtogroup Display
 */

/*@{*/

/*-------------------------------------------------------------------------*/
/*                         start of code                                   */
/*-------------------------------------------------------------------------*/

/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief control backlight
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
void LcdBackLight(u_char Mode)
{
    if (Mode==LCD_BACKLIGHT_ON)
    {
        sbi(LCD_BL_PORT, LCD_BL_BIT);   // Turn on backlight
    }

    if (Mode==LCD_BACKLIGHT_OFF)
    {
        cbi(LCD_BL_PORT, LCD_BL_BIT);   // Turn off backlight
    }
}

/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief Write a single character on the LCD
 *
 * Writes a single character on the LCD on the current cursor position
 *
 * \param LcdChar character to write
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
void LcdChar(char MyChar)
{
    LcdWriteByte(WRITE_DATA, MyChar);
}


/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief Low-level initialisation function of the LCD-controller
 *
 * Initialise the controller and send the User-Defined Characters to CG-RAM
 * settings: 4-bit interface, cursor invisible and NOT blinking
 *           1 line dislay, 10 dots high characters
 *
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
 void LcdLowLevelInit()
{
    u_char i;

    NutDelay(140);                               // wait for more than 140 ms after Vdd rises to 2.7 V

    for (i=0; i<3; ++i)
    {
        LcdWriteNibble(WRITE_COMMAND, 0x33);      // function set: 8-bit mode; necessary to guarantee that
        NutDelay(4);                              // SIR starts up always in 5x10 dot mode
    }

    LcdWriteNibble(WRITE_COMMAND, 0x22);        // function set: 4-bit mode; necessary because KS0070 doesn't
    NutDelay(1);                                // accept combined 4-bit mode & 5x10 dot mode programming

    //LcdWriteByte(WRITE_COMMAND, 0x24);        // function set: 4-bit mode, 5x10 dot mode, 1-line
    LcdWriteByte(WRITE_COMMAND, 0x28);          // function set: 4-bit mode, 5x7 dot mode, 2-lines
    NutDelay(5);

    LcdWriteByte(WRITE_COMMAND, 0x0C);          // display ON/OFF: display ON, cursor OFF, blink OFF
    NutDelay(5);

    LcdWriteByte(WRITE_COMMAND, 0x01);          // display clear
    NutDelay(5);

    LcdWriteByte(WRITE_COMMAND, 0x06);          // entry mode set: increment mode, entire shift OFF


    LcdWriteByte(WRITE_COMMAND, 0x80);          // DD-RAM address counter (cursor pos) to '0'
}


/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief Low-level routine to write a byte to LCD-controller
 *
 * Writes one byte to the LCD-controller (by  calling LcdWriteNibble twice)
 * CtrlState determines if the byte is written to the instruction register
 * or to the data register.
 *
 * \param CtrlState destination: instruction or data
 * \param LcdByte byte to write
 *
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
static void LcdWriteByte(u_char CtrlState, u_char LcdByte)
{
    LcdWaitBusy();                      // see if the controller is ready to receive next byte
    LcdWriteNibble(CtrlState, LcdByte & 0xF0);
    LcdWriteNibble(CtrlState, LcdByte << 4);

}

/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief Low-level routine to write a nibble to LCD-controller
 *
 * Writes a nibble to the LCD-controller (interface is a 4-bit databus, so
 * only 4 databits can be send at once).
 * The nibble to write is in the upper 4 bits of LcdNibble
 *
 * \param CtrlState destination: instruction or data
 * \param LcdNibble nibble to write (upper 4 bits in this byte
 *
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
static void LcdWriteNibble(u_char CtrlState, u_char LcdNibble)
{
    outp((inp(LCD_DATA_DDR) & 0x0F) | 0xF0, LCD_DATA_DDR);  // set data-port to output again

    outp((inp(LCD_DATA_PORT) & 0x0F) | (LcdNibble & 0xF0), LCD_DATA_PORT); // prepare databus with nibble to write

    if (CtrlState == WRITE_COMMAND)
    {
        cbi(LCD_RS_PORT, LCD_RS);     // command: RS low
    }
    else
    {
        sbi(LCD_RS_PORT, LCD_RS);     // data: RS high
    }

    sbi(LCD_EN_PORT, LCD_EN);

    asm("nop\n\tnop");                    // small delay

    cbi(LCD_EN_PORT, LCD_EN);
    cbi(LCD_RS_PORT, LCD_RS);
    outp((inp(LCD_DATA_DDR) & 0x0F), LCD_DATA_DDR);           // set upper 4-bits of data-port to input
    outp((inp(LCD_DATA_PORT) & 0x0F) | 0xF0, LCD_DATA_PORT);  // enable pull-ups in data-port
}

/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
/*!
 * \brief Low-level routine to see if the controller is ready to receive
 *
 * This routine repeatetly reads the databus and checks if the highest bit (bit 7)
 * has become '0'. If a '0' is detected on bit 7 the function returns.
 *
 */
/* อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ */
static void LcdWaitBusy()
{
    u_char Busy = 1;
	u_char LcdStatus = 0;

    cbi (LCD_RS_PORT, LCD_RS);              // select instruction register

    sbi (LCD_RW_PORT, LCD_RW);              // we are going to read

    while (Busy)
    {
        sbi (LCD_EN_PORT, LCD_EN);          // set 'enable' to catch 'Ready'

        asm("nop\n\tnop");                  // small delay
        LcdStatus =  inp(LCD_IN_PORT);      // LcdStatus is used elsewhere in this module as well
        Busy = LcdStatus & 0x80;            // break out of while-loop cause we are ready (b7='0')
    }

    cbi (LCD_EN_PORT, LCD_EN);              // all ctrlpins low
    cbi (LCD_RS_PORT, LCD_RS);
    cbi (LCD_RW_PORT, LCD_RW);              // we are going to write
}

void writeLcd(char text1[], char text2[])
{
	int i;
	for ( i=0; i<80; i++ )
	{
		LcdChar( ' ' );
	}
	lcd_writeLine1(text1);
	lcd_writeLine2(text2);
}

// write upper line
void lcd_writeLine1 ( char text1[] )
{
	int i;
	// eerst de eerste 8 karakters = regel 1
	//lcdCommand(0x80);			// first position, line 1, adress $00
	for ( i=0; i<40; i++ )
	{
		LcdChar( text1[i] );
	}
} 

// write second line
void lcd_writeLine2 ( char text2[] )
{
	int i;
	// dan de tweede 8 karakters = regel 2
	//lcdCommand(0xC0);			// first position, line 2, adress $40

	for ( i=0; i<10; i++ )
	{
		LcdChar( text2[i] );
	}
	
	for ( i=0; i<30; i++ )
	{
		LcdChar( ' ' );
	}
} 
 
/* ---------- end of module ------------------------------------------------ */

/*@}*/
