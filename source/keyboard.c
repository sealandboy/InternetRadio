/*! \file
 * keyboard.c contains the low-level keyboard scan and the
 * interfacing with NutOS (signalling)
 *  Copyright STREAMIT, 2010.
 *  \version 1.0
 *  \date 26 september 2003
 */


#define LOG_MODULE  LOG_KEYBOARD_MODULE

#include <sys/atom.h>
#include <sys/event.h>

//#pragma text:appcode

#include "keyboard.h"
#include "portio.h"
#include "system.h"

/*-------------------------------------------------------------------------*/
/* local defines                                                          */
/*-------------------------------------------------------------------------*/
/*
 *  definition of raw keys as found in keyboardscan
 *  Note that these 16-bit values are remapped before
 *  the application uses them
 */
#define RAW_KEY_01         0xFFFB
#define RAW_KEY_02         0xFFFD
#define RAW_KEY_03         0xFF7F
#define RAW_KEY_04         0xFFF7
#define RAW_KEY_05         0xFFFE
#define RAW_KEY_ALT        0xFFBF

#define RAW_KEY_ESC        0xFFEF
#define RAW_KEY_UP         0xF7FF
#define RAW_KEY_OK         0xFFDF
#define RAW_KEY_LEFT       0xFEFF
#define RAW_KEY_DOWN       0xFBFF
#define RAW_KEY_RIGHT      0xFDFF

#define RAW_KEY_POWER      0xEFFF

#define RAW_KEY_SETUP      0xFFCF       // combine 'ESCAPE' (0xFFEF') with 'OK' (0xFFDF)

/*-------------------------------------------------------------------------*/
/* local variable definitions                                              */
/*-------------------------------------------------------------------------*/
static HANDLE  hKBEvent;
static u_short KeyFound;        // use short for 4 nibbles (4 colums)
static u_char KeyBuffer[KB_BUFFER_SIZE];
static u_short HoldCounter;
static u_char KbState;
static u_char KeyRepeatArray[KEY_NROF_KEYS];

/*-------------------------------------------------------------------------*/
/* local routines (prototyping)                                            */
/*-------------------------------------------------------------------------*/
static void KbClearEvent(HANDLE*);
static u_char KbRemapKey(u_short LongKey);


/*!
 * \addtogroup Keyboard
 */

/*@{*/

/*-------------------------------------------------------------------------*/
/*                         start of code                                   */
/*-------------------------------------------------------------------------*/




/* ����������������������������������������������������������������������� */
/*!
 * \brief Clear the eventbuffer of this module
 *
 * This routine is called during module initialization.
 *
 * \param *pEvent pointer to the event queue
 */
/* ����������������������������������������������������������������������� */
static void KbClearEvent(HANDLE *pEvent)
{
    NutEnterCritical();

    *pEvent = 0;

    NutExitCritical();
}

/* ����������������������������������������������������������������������� */
/*!
 * \brief Low-level keyboard scan
 *
 * KbScan is called each 4.44 msec from MainBeat interrupt
 * Remember: pressed key gives a '0' on KB_IN_READ
 *
 * After each keyboard-scan, check for a valid MMCard
 */
/* ����������������������������������������������������������������������� */
void KbScan()
{
    u_char KeyNibble0, KeyNibble1, KeyNibble2, KeyNibble3;

    /*
     *  we must scan 4 colums, 2 in PORTG and 2 in PORTD
     */

#ifndef USE_JTAG
    // scan keys in COL 0
    cbi (KB_OUT_WRITE_A, KB_COL_0);
    asm("nop\n\tnop");                    // small delay
    KeyNibble0 = inp(KB_IN_READ) & KB_ROW_MASK;
    sbi (KB_OUT_WRITE_A, KB_COL_0);

    // scan keys in COL 1
    cbi (KB_OUT_WRITE_A, KB_COL_1);
    asm("nop\n\tnop");                    // small delay
    KeyNibble1 = inp(KB_IN_READ) & KB_ROW_MASK;
    sbi (KB_OUT_WRITE_A, KB_COL_1);

    // scan keys in COL 2
    cbi (KB_OUT_WRITE_B, KB_COL_2);
    asm("nop\n\tnop");                    // small delay
    KeyNibble2 = inp(KB_IN_READ) & KB_ROW_MASK;
    sbi (KB_OUT_WRITE_B, KB_COL_2);

    // scan keys in COL 3
    cbi (KB_OUT_WRITE_B, KB_COL_3);
    asm("nop\n\tnop");                    // small delay
    KeyNibble3 = inp(KB_IN_READ) & KB_ROW_MASK;
    sbi (KB_OUT_WRITE_B, KB_COL_3);


    /*
     *  we want to detect exactly 1 key in exactly 1 colom
     *  exception is the combination of VOLMIN & POWER (-> SETUP)
     *  meaning: Keynibble0==[0000 1011] (KEY_VOLMIN) & KeyNibble1==[0111 0000] (KEY_POWER)
     */

    /*
     *  put all 4 seperate nibbles in place in 'KeyFound'
     *
     *  KeyNibble0 on b3...b0  (col 0)
     *  KeyNibble1 on b7...b4  (col 1)
     *  KeyNibble2 on b11..b8  (col 2)
     *  KeyNibble3 on b15..b12 (col 3)
     */

    KeyFound =  ((KeyNibble0>>4) & 0x000F);     // b7..b4 in 'KeyNibble0' to b3...b0  in 'KeyFound' >> shift 4 right
    KeyFound |= (KeyNibble1 & 0x00F0);          // b7..b4 in 'KeyNibble1' to b7...b4  in 'KeyFound' -- do nothing
    KeyFound |= ((KeyNibble2<<4) & 0x0F00);     // b7..b4 in 'KeyNibble2' to b11..b8  in 'KeyFound' << shift 4 left
    KeyFound |= ((KeyNibble3<<8) & 0xF000);     // b7..b4 in 'KeyNibble3' to b15..b12 in 'KeyFound' << shift 8 left

    KeyBuffer[0] = KbRemapKey(KeyFound);

#endif  // USE_JTAG

}

/* ����������������������������������������������������������������������� */
/*!
 * \brief Remap the 16-bit value for the active key to an 8-bit value
 *
 */
/* ����������������������������������������������������������������������� */
static u_char KbRemapKey(u_short LongKey)
{
    switch (LongKey)
    {
        case RAW_KEY_01:        return(KEY_01);
        case RAW_KEY_02:        return(KEY_02);
        case RAW_KEY_03:        return(KEY_03);
        case RAW_KEY_04:        return(KEY_04);
        case RAW_KEY_05:        return(KEY_05);
        case RAW_KEY_ALT:       return(KEY_ALT);

        case RAW_KEY_ESC:       return(KEY_ESC);
        case RAW_KEY_UP:        return(KEY_UP);
        case RAW_KEY_OK:        return(KEY_OK);
        case RAW_KEY_LEFT:      return(KEY_LEFT);
        case RAW_KEY_DOWN:      return(KEY_DOWN);
        case RAW_KEY_RIGHT:     return(KEY_RIGHT);

        case RAW_KEY_POWER:     return(KEY_POWER);
        case RAW_KEY_SETUP:     return(KEY_SETUP);      // combined key

        default:                return(KEY_UNDEFINED);
    }
}

/* ����������������������������������������������������������������������� */
/*!
 * \brief Return the repeating property for this key
 *
 * \return 'TRUE' in case the key was repeating, 'FALSE' if not
 *
 */
/* ����������������������������������������������������������������������� */
static u_char KbKeyIsRepeating(u_short Key)
{
    return(KeyRepeatArray[KbRemapKey(Key)]==KEY_REPEAT);
}

/* ����������������������������������������������������������������������� */
/*!
 * \brief set the property of this key to repeating or not-repeating
 *
 */
/* ����������������������������������������������������������������������� */
void KbSetKeyRepeating(u_char Key, u_char Property)
{
    // check arguments
    if (((Property==KEY_REPEAT) || (Property==KEY_NO_REPEAT)) && (Key < KEY_NROF_KEYS))
    {
        KeyRepeatArray[Key]=Property;
    }
}

/* ����������������������������������������������������������������������� */
/*!
 * \brief Wait until an event was pushed on the eventqueue for this module
 *
 * This routine provides the event interface for other Luks-modules
 *
 * \param dwTimeout time in milisecs that this routine should wait before
 * it will return with KB_ERROR
 *
 * \return KB_OK in case an event was found
 * \return KB_ERROR in case no event was found (return due to timeout)
 */
/* ����������������������������������������������������������������������� */
int KbWaitForKeyEvent(u_long dwTimeout)
{

    int nError = KB_OK;
    int nTimeout;

    nTimeout = NutEventWait(&hKBEvent, dwTimeout);
    if (nTimeout == -1)
    {
        nError = KB_ERROR;
    }

    return(nError);
}

/* ����������������������������������������������������������������������� */
/*!
 * \brief Return the databyte that was receeived in the IR-stream
 *
 * In case a valid key is found in the keyboard scan, the key-code is
 * stored in the keyboardbuffer. This routine returns the first available
 * valid key in this buffer

 * \return the keycode that was found by the keyboard scan
 *
 * \todo implement a key-buffer for this routine
 */
/* ����������������������������������������������������������������������� */
u_char KbGetKey()
{
    return(KeyBuffer[0]);
}

/*!
 * \brief inject a virtual key into the system
 *
 */
void KbInjectKey(u_char VirtualKey)
{
    KeyBuffer[0]=VirtualKey;
    NutEventPostFromIrq(&hKBEvent);   // 'valid key' detected -> generate Event
}
/* ����������������������������������������������������������������������� */
/*!
 * \brief Initialise the Keyboard module
 *
 *
 * - initialise the keyboard read- and write port
 * - flush the keyboardbuffer
 * - flush the eventqueue for this module
 *
 * \note PORTF uses internal pull-ups. That's why a '1' is read
 * when no key is pressed. Use negative logic to detect keys.
 * So default state of the colums is '1'
 */
/* ����������������������������������������������������������������������� */
void KbInit()
{
    u_char i;

    sbi (KB_OUT_WRITE_A, KB_COL_0);
    sbi (KB_OUT_WRITE_A, KB_COL_1);
    sbi (KB_OUT_WRITE_B, KB_COL_2);
    sbi (KB_OUT_WRITE_B, KB_COL_3);

    KbState = KB_IDLE;
    KeyFound = KEY_NO_KEY;

    KbClearEvent(&hKBEvent);

    for (i=0;i<KB_BUFFER_SIZE;++i)
    {
        KeyBuffer[i] = (u_char)KEY_NO_KEY;
    }

    for (i=0; i<KEY_NROF_KEYS; ++i)
    {
        KeyRepeatArray[i]=KEY_NO_REPEAT;
    }

    HoldCounter=0;

    // arrow keys are repeating keys by default
    KbSetKeyRepeating(KEY_UP, KEY_REPEAT);
    KbSetKeyRepeating(KEY_DOWN, KEY_REPEAT);
    KbSetKeyRepeating(KEY_LEFT, KEY_REPEAT);
    KbSetKeyRepeating(KEY_RIGHT, KEY_REPEAT);
}
/* ---------- end of module ------------------------------------------------ */

/*@}*/
