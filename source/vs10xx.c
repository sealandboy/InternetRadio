/*
 * Copyright (C) 2003 by Pavel Chromy. All rights reserved.
 * Copyright (C) 2001-2003 by egnite Software GmbH. All rights reserved.
 * Copyright (C) 2003-2004 by Streamit. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY EGNITE SOFTWARE GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL EGNITE
 * SOFTWARE GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 * -
 *
 * This software has been inspired by all the valuable work done by
 * Jesper Hansen <jesperh@telia.com>. Many thanks for all his help.
 */

/*
 * [COPYRIGHT]  Copyright (C) STREAMIT BV
 *
 *
 */
#define LOG_MODULE  LOG_VS10XX_MODULE

#include <stdlib.h>

#include <sys/atom.h>
#include <sys/event.h>
#include <sys/timer.h>
#include <sys/heap.h>

#include <dev/irqreg.h>

#include <sys/bankmem.h>

#if (NUTOS_VERSION >= 433)
    #include <cpu_load.h>
#endif


#include "system.h"
#include "vs10xx.h"
#include "platform.h"
#include "log.h"
#include "portio.h"    // for debug purposes only
#include "spidrv.h"    // for debug purposes only
#include "watchdog.h"


/*-------------------------------------------------------------------------*/
/* global variable definitions                                             */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* local variable definitions                                              */
/*-------------------------------------------------------------------------*/

#define LOW         0
#define HIGH        1

#define MONO        0
#define STEREO      1

#define VsDeselectVs()  SPIdeselect()
#define VsSelectVs()    SPIselect(SPI_DEV_VS10XX)


/*-------------------------------------------------------------------------*/
/* local variable definitions                                              */
/*-------------------------------------------------------------------------*/
static volatile u_char vs_status = VS_STATUS_STOPPED;
static u_short g_vs_type;
static u_char VsPlayMode;


static void VsLoadProgramCode(void);

/*-------------------------------------------------------------------------*/
/* local routines (prototyping)                                            */
/*-------------------------------------------------------------------------*/

#define CODE_SIZE 437
static prog_char atab[CODE_SIZE] = { /* Register addresses */
    7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6
};

prog_int dtab[CODE_SIZE] = { /* Data to write */
    0x8030, 0x0030, 0x0717, 0xb080, 0x3c17, 0x0006, 0x5017, 0x3f00,
    0x0024, 0x0006, 0x2016, 0x0012, 0x578f, 0x0000, 0x10ce, 0x2912,
    0x9900, 0x0000, 0x004d, 0x4080, 0x184c, 0x0006, 0x96d7, 0x2800,
    0x0d55, 0x0000, 0x0d48, 0x0006, 0x5b50, 0x3009, 0x0042, 0xb080,
    0x8001, 0x4214, 0xbc40, 0x2818, 0xc740, 0x3613, 0x3c42, 0x3e00,
    0xb803, 0x0014, 0x1b03, 0x0015, 0x59c2, 0x6fd6, 0x0024, 0x3600,
    0x9803, 0x2812, 0x57d5, 0x0000, 0x004d, 0x2800, 0x2b40, 0x36f3,
    0x0024, 0x804c, 0x3e10, 0x3814, 0x3e10, 0x780a, 0x3e13, 0xb80d,
    0x3e03, 0xf805, 0x0006, 0x5595, 0x3009, 0x1415, 0x001b, 0xffd4,
    0x0003, 0xffce, 0x0001, 0x000a, 0x2400, 0x16ce, 0xb58a, 0x0024,
    0xf292, 0x9400, 0x6152, 0x0024, 0xfe02, 0x0024, 0x48b2, 0x0024,
    0x454a, 0xb601, 0x36f3, 0xd805, 0x36f3, 0x980d, 0x36f0, 0x580a,
    0x2000, 0x0000, 0x36f0, 0x1814, 0x8061, 0x3613, 0x0024, 0x3e12,
    0xb817, 0x3e12, 0x3815, 0x3e05, 0xb814, 0x3625, 0x0024, 0x0000,
    0x800a, 0x3e10, 0xb803, 0x4194, 0xb805, 0x3e11, 0x0024, 0x3e11,
    0xb807, 0x3e14, 0x7812, 0x3e14, 0xf80d, 0x3e03, 0xf80e, 0x0006,
    0x0051, 0x2800, 0x24d5, 0x0000, 0x0024, 0xb888, 0x0012, 0x6404,
    0x0405, 0x0000, 0x0024, 0x2800, 0x2158, 0x4094, 0x0024, 0x2400,
    0x2102, 0x0000, 0x0024, 0x6498, 0x0803, 0xfe56, 0x0024, 0x48b6,
    0x0024, 0x4dd6, 0x0024, 0x3a10, 0xc024, 0x32f0, 0xc024, 0xfe56,
    0x0024, 0x48b6, 0x0024, 0x4dd6, 0x0024, 0x4384, 0x4483, 0x6396,
    0x888c, 0xf400, 0x40d5, 0x3d00, 0x8024, 0x0006, 0x0091, 0x003f,
    0xfec3, 0x0006, 0x0053, 0x3101, 0x8024, 0xfe60, 0x0024, 0x48be,
    0x0024, 0xa634, 0x0c03, 0x4324, 0x0024, 0x4284, 0x2c02, 0x0006,
    0x0011, 0x2800, 0x24d8, 0x3100, 0x8024, 0x0006, 0x5011, 0x3900,
    0x8024, 0x0006, 0x0011, 0x3100, 0x984c, 0x4284, 0x904c, 0xf400,
    0x4088, 0x2800, 0x2845, 0x0000, 0x0024, 0x3cf0, 0x3840, 0x3009,
    0x3841, 0x3009, 0x3810, 0x2000, 0x0000, 0x0000, 0x2788, 0x3009,
    0x1bd0, 0x2800, 0x2880, 0x3009, 0x1b81, 0x34f3, 0x1bcc, 0x36f3,
    0xd80e, 0x36f4, 0xd80d, 0x36f4, 0x5812, 0x36f1, 0x9807, 0x36f1,
    0x1805, 0x36f0, 0x9803, 0x3405, 0x9014, 0x36f3, 0x0024, 0x36f2,
    0x1815, 0x2000, 0x0000, 0x36f2, 0x9817, 0x80ad, 0x3e12, 0xb817,
    0x3e12, 0x3815, 0x3e05, 0xb814, 0x3615, 0x0024, 0x0000, 0x800a,
    0x3e10, 0x7802, 0x3e10, 0xf804, 0x3e11, 0x7810, 0x3e14, 0x7812,
    0x2913, 0xc980, 0x3e14, 0xc024, 0x2913, 0xc980, 0x4088, 0x184c,
    0xf400, 0x4005, 0x0000, 0x18c0, 0x6400, 0x0024, 0x0000, 0x1bc0,
    0x2800, 0x3095, 0x0030, 0x0310, 0x2800, 0x3f80, 0x3801, 0x4024,
    0x6400, 0x0024, 0x0000, 0x1a40, 0x2800, 0x3755, 0x0006, 0x55d0,
    0x0000, 0x7d03, 0xb884, 0x184c, 0x3009, 0x3805, 0x3009, 0x0000,
    0xff8a, 0x0024, 0x291d, 0x7b00, 0x48b2, 0x0024, 0x0000, 0x1841,
    0x0006, 0x5010, 0x408a, 0xb844, 0x2900, 0x1300, 0x4088, 0x0024,
    0x3000, 0x1bcc, 0x6014, 0x0024, 0x0030, 0x0351, 0x2800, 0x36d5,
    0x0000, 0x0024, 0x0006, 0x0011, 0x3100, 0x0024, 0x0030, 0x0351,
    0x3800, 0x0024, 0x2800, 0x3f80, 0x3901, 0x4024, 0x6400, 0x0024,
    0x0030, 0x03d0, 0x2800, 0x3f55, 0x0000, 0x7d03, 0x0006, 0x55d0,
    0xb884, 0x184c, 0x3009, 0x3805, 0x3009, 0x0000, 0xff8a, 0x0024,
    0x291d, 0x7b00, 0x48b2, 0x0024, 0x408a, 0x9bcc, 0x0000, 0x1841,
    0x2800, 0x3b55, 0x0006, 0x5010, 0x689a, 0x0024, 0x3000, 0x0024,
    0x6014, 0x0024, 0x0030, 0x0392, 0x2800, 0x3e85, 0x0006, 0x0091,
    0x0006, 0x0011, 0x0000, 0x1852, 0x0006, 0x0053, 0xb880, 0x2400,
    0x0006, 0x0091, 0x3804, 0x8024, 0x0030, 0x0392, 0x3b00, 0x0024,
    0x3901, 0x4024, 0x2800, 0x3f80, 0x3a01, 0x4024, 0x3801, 0x4024,
    0xb880, 0x1bd3, 0x36f4, 0x5812, 0x36f1, 0x5810, 0x36f0, 0xd804,
    0x36f0, 0x5802, 0x3405, 0x9014, 0x36f3, 0x0024, 0x36f2, 0x1815,
    0x2000, 0x0000, 0x36f2, 0x9817, 0x0030
};

/*!
 * \addtogroup VS1003B
 */

/*@{*/

/*-------------------------------------------------------------------------*/
/*                         start of code                                   */
/*-------------------------------------------------------------------------*/


/*!
 * \brief Write a specified number of bytes to the VS10XX data interface.
 *
 * Decoder interrupts must have been disabled before calling this function.
 */
static void VsSdiWrite(CONST u_char * data, u_short len)
{
    VsSelectVs();

    while (len--)
    {
        SPIputByte(*data);
        data++;
    }

    VsDeselectVs();
    return;
}

/*!
 * \brief Write a specified number of bytes from program space to the
 *        VS10XX data interface.
 *
 * This function is similar to VsSdiWrite() except that the data is
 * located in program space.
 */
static void VsSdiWrite_P(PGM_P data, u_short len)
{
    VsSelectVs();

    while (len--)
    {
        SPIputByte(PRG_RDB(data));
        data++;
    }

    VsDeselectVs();
    return;
}


/*!
 * \brief Write to a decoder register.
 *
 * Decoder interrupts must have been disabled before calling this function.
 */
void VsRegWrite(u_char reg, u_short data)
{
    u_char spimode;

    spimode = SPIgetmode();
    SPImode(SPEED_SLOW);

    VsSelectVs();

    cbi(VS_XCS_PORT, VS_XCS_BIT);

    SPIputByte(VS_OPCODE_WRITE);
    SPIputByte(reg);
    SPIputByte((u_char) (data >> 8));
    SPIputByte((u_char) data);

    sbi(VS_XCS_PORT, VS_XCS_BIT);

    VsDeselectVs();

    SPImode(spimode);

    return;
}

/*!
 * \brief determine if the stream is valid. If true, returns value; if false returns 0
 *
 */
u_short VsStreamValid(void)
{
    u_short value;
    u_short result;

    value = VsRegInfo(VS_HDAT1_REG);

    if (value > 0xFFE0)
    {
        value = 0xFFE0;
    }

    switch (value)
    {
        case 0x7665: /* WAV */
        case 0x4154: /* AAC DTS */
        case 0x4144: /* AAC ADIF */
        case 0x4D34: /* AAC .mp4 / .m4a */
        case 0x574D: /* WMA without broadcast patch*/
            {
                result= (g_vs_type==VS_VS1003? 0 : value);
            }
            break;

        case 0x576d: /* WMA with broadcast patch*/
        case 0x4D54: /* MIDI */
        case 0xFFE0: /* MP3 */
            {
                result=value;
            }
            break;

        default:
            {
                result=0;
            }
    }

    return(result);
}
/*
 * \brief Read from a register.
 *
 * Decoder interrupts must have been disabled before calling this function.
 *
 * \return Register contents.
 */
static u_short VsRegRead(u_char reg)
{
    u_short data;
    u_char spimode;

    spimode = SPIgetmode();
    SPImode(SPEED_SLOW);

    VsSelectVs();

    cbi(VS_XCS_PORT, VS_XCS_BIT);

    SPIputByte(VS_OPCODE_READ);
    SPIputByte(reg);

    data=SPIgetByte()<<8;           // get MSB
    data |= SPIgetByte();           // get LSB

    sbi(VS_XCS_PORT, VS_XCS_BIT);

    VsDeselectVs();
    SPImode(spimode);

    return(data);
}

/*!
 * \brief read data from a specified register from the VS10XX
 *
 */
u_short VsRegInfo(u_char reg)
{
    u_char ief;
    u_short value;

    ief = VsPlayerInterrupts(0);
    value = VsRegRead(reg);
    VsPlayerInterrupts(ief);

    return(value);
}


/*!
 * \brief Enable or disable player interrupts.
 *
 * This routine is typically used by applications when dealing with
 * unprotected buffers.
 *
 * \param enable Disables interrupts when zero. Otherwise interrupts
 *               are enabled.
 *
 * \return Zero if interrupts were disabled before this call.
 */
u_char VsPlayerInterrupts(u_char enable)
{
    u_char rc;

    NutEnterCritical();
    rc = (inb(EIMSK) & _BV(VS_DREQ_BIT)) != 0;
    if (enable)
    {
        sbi(EIMSK, VS_DREQ_BIT);
    }
    else
    {
        cbi(EIMSK, VS_DREQ_BIT);
    }
    NutExitCritical();

    return(rc);
}

/*
 * \brief Feed the decoder with data.
 *
 * This function serves two purposes:
 * - It is called by VsPlayerKick() to initially fill the decoder buffer.
 * - It is used as an interrupt handler for the decoder.
 *
 * Note that although this routine is an ISR, it is called from 'VsPlayerKick' as well
 */
static void VsPlayerFeed(void *arg)
{
    u_short j = 32;
    u_char ief;

    char *bp;
    size_t consumed;
    size_t available;

    // leave if not running.
    if ((vs_status != VS_STATUS_RUNNING) || (bit_is_clear(VS_DREQ_PIN, VS_DREQ_BIT)))
    {
        return;
    }

    /*
     * We are hanging around here some time and may block other important
     * interrupts. Disable decoder interrupts and enable global interrupts.
     */
    ief = VsPlayerInterrupts(0);

    sei();

    bp = 0;
    consumed = 0;
    available = 0;

    /*
     * Feed the decoder with j bytes or we ran out of data.
     */
    VsSelectVs();

    do
    {
        if (consumed >= available)
        {
            // Commit previously consumed bytes.
            if (consumed)
            {
                NutSegBufReadCommit(consumed);
                consumed = 0;
            }
            // All bytes consumed, request new.
            bp = NutSegBufReadRequest(&available);
            if (available == 0)
            {
                /* End of stream. */
                vs_status = VS_STATUS_EOF;
                break;
            }
        }
        if (available != 0) // We have some data in the buffer, feed it.
        {
            SPIputByte(*bp);
            bp++;
            consumed++;
        }
        /*
         * appearantly DREQ goes low when less then 32 byte are available
         * in the internal buffer (2048 bytes)
         */

        /* Allow 32 bytes to be sent as long as DREQ is set. This includes the one in progress */
        if (bit_is_set(VS_DREQ_PIN, VS_DREQ_BIT))
        {
            j = 32;
        }
    } while (--j);          // bug solved: j-- counts one too many....

    VsDeselectVs();

    /* Finally re-enable the producer buffer. */
    NutSegBufReadLast(consumed);
    VsPlayerInterrupts(ief);
}


/*!
 * \brief Start playback.
 *
 * This routine will send the first MP3 data bytes to the
 * decoder. The data buffer
 * should have been filled before calling this routine.
 *
 * Decoder interrupts will be enabled.
 *
 * \return 0 on success, -1 otherwise.
 */
int VsPlayerKick(void)
{
    /*
     * Start feeding the decoder with data.
     */
    if (vs_status != VS_STATUS_RUNNING)
    {
        VsPlayerInterrupts(0);
        /*
         *  for the VS1003 we need an extra reset
         *  here before we start playing a stream...
         */
//        VsPlayerSetMode(VS_SM_RESET);
//        NutDelay(10);
//        LogMsg_P(LOG_DEBUG,PSTR("Kick: CLOCKF = [0x%02X]"),VsRegRead(VS_CLOCKF_REG));
//        LogMsg_P(LOG_DEBUG,PSTR("Kick: CLOCKF = [0x%02X]"),VsRegRead(VS_CLOCKF_REG));
//        LogMsg_P(LOG_DEBUG,PSTR("Kick: CLOCKF = [0x%02X]"),VsRegRead(VS_CLOCKF_REG));
//        LogMsg_P(LOG_DEBUG,PSTR("Kick: CLOCKF = [0x%02X]"),VsRegRead(VS_CLOCKF_REG));
//        LogMsg_P(LOG_DEBUG,PSTR("Kick: CLOCKF = [0x%02X]"),VsRegRead(VS_CLOCKF_REG));

        VsLoadProgramCode();
        vs_status = VS_STATUS_RUNNING;
        VsPlayerFeed(NULL);
        VsPlayerInterrupts(1);
    }
    return(0);
}

/*!
 * \brief Stops the playback.
 *
 * This routine will stops the MP3 playback, VsPlayerKick() may be used
 * to resume the playback.
 *
 * \return 0 on success, -1 otherwise.
 */
int VsPlayerStop(void)
{
    u_char ief;

    ief = VsPlayerInterrupts(0);
    /* Check whether we need to stop at all to not overwrite other than running status */
    if (vs_status == VS_STATUS_RUNNING)
        vs_status = VS_STATUS_STOPPED;
    VsPlayerInterrupts(ief);

    return(0);
}


/*!
 * \brief Initialize the VS10xx hardware interface.
 *
 * \return 0 on success, -1 otherwise.
 */
int VsPlayerInit(void)
{

    /* Disable decoder interrupts. */

    VsPlayerInterrupts(0);

    /* Keep decoder in reset state. */
    cbi(VS_RESET_PORT, VS_RESET_BIT);

    /* Set VS10XX chip select output inactive for SCI bus (high) */
    sbi(VS_XCS_PORT, VS_XCS_BIT);

    /* Set SCK output low. */
    cbi(VS_SCK_PORT, VS_SCK_BIT);
    sbi(VS_SCK_DDR, VS_SCK_BIT);


    /*
     * Init SPI mode to no interrupts, enabled, MSB first, master mode,
     * rising clock and fosc/8 clock speed. Send an initial byte to
     * make sure SPIF is set. Note, that the decoder reset line is still
     * active.
     */
    NutDelay(4);

    SPImode(SPEED_SLOW);

    vs_status = VS_STATUS_STOPPED;

    /* Release decoder reset line. */
    sbi(VS_RESET_PORT, VS_RESET_BIT);

    /* Wait until DREQ is active
     * Write 0x9800 to SCI_CLOCKF
     * do another register write
     * wait at least 11000 clockcycles
     */
    NutDelay(4);

    /* Read the status register to determine the VS type. */
    g_vs_type = (VsRegRead(VS_STATUS_REG) >> 4) & 7;

    /* Force frequency change (see datasheet). */
    switch (g_vs_type)
    {
        case VS_VS1003:
            {
                VsRegWrite(VS_CLOCKF_REG, 0xE000); // 4.5x
                break;
            }
        default:
            {
                VsRegWrite(VS_CLOCKF_REG, 0x9800); // 2x
                break;
            }
    }

    NutDelay(50);

    // Datasheet requires 2 write instructions before speeding up SPI interface
    VsPlayerSetMode(0);
    VsSetVolume(0,0);

    NutDelay(50);

    // now switch to new speed...
    switch (g_vs_type)
    {
        case VS_VS1003:
        case VS_VS1053:
            {
                SPImode(SPEED_ULTRA_FAST);
                break;
            }
        case VS_VS1011e:
        default:
            {
                SPImode(SPEED_FAST);
                break;
            }
    }

    /* Register the interrupt routine */
    NutRegisterIrqHandler(&sig_INTERRUPT6, VsPlayerFeed, NULL);

    /* Rising edge will generate interrupts. */
    NutIrqSetMode(&sig_INTERRUPT6, NUT_IRQMODE_RISINGEDGE);

    /* Clear any spurious interrupt. */
    outp(BV(VS_DREQ_BIT), EIFR);

    return(0);
}

/*!
 * \brief Software reset the decoder.
 *
 * This function is typically called after VsPlayerInit() and at the end
 * of each track.
 *
 * \param mode Any of the following flags may be or'ed
 * - VS_SM_DIFF Left channel inverted.
 * - VS_SM_FFWD Fast forward.
 * - VS_SM_RESET Force hardware reset.
 * - VS_SM_PDOWN Switch to power down mode.
 * - VS_SM_BASS Bass/treble enhancer.
 *
 * \return 0 on success, -1 otherwise.
 */
int VsPlayerReset(u_short mode)
{
    /* Disable decoder interrupts and feeding. */
    VsPlayerInterrupts(0);
    vs_status = VS_STATUS_STOPPED;

    /* Software reset, set modes of decoder. */
    VsPlayerSetMode(VS_SM_RESET | mode);
    NutDelay(10);

    /* Clear any spurious interrupts. */
    outp(BV(VS_DREQ_BIT), EIFR);

    return(0);
}

/*!
 * \brief Set mode register of the decoder.
 *
 * \param mode Any of the following flags may be or'ed
 * - VS_SM_DIFF Left channel inverted.
 * - VS_SM_FFWD Fast forward.
 * - VS_SM_RESET Software reset.
 * - VS_SM_PDOWN Switch to power down mode.
 * - VS_SM_BASS Bass/treble enhancer.
 *
 * \return 0 on success, -1 otherwise.
 */
int VsPlayerSetMode(u_short mode)
{
    u_char ief;

    ief = VsPlayerInterrupts(0);
    /*
     *  We need to be sure that the way of interfacing
     *  is not corrupted by setting some new mode
     *  We need to have SM_SDINEW & SM_SDISHARE set to '1'
     *  at all times
     */
    mode |= (VS_SM_SDISHARE | VS_SM_SDINEW);
    VsRegWrite(VS_MODE_REG, mode);
    VsPlayerInterrupts(ief);

    return(0);
}

/*!
 * \brief Returns status of the player.
 *
 * \return Any of the following value:
 * - VS_STATUS_STOPPED Player is ready to be started by VsPlayerKick().
 * - VS_STATUS_RUNNING Player is running.
 * - VS_STATUS_EOF Player has reached the end of a stream after VsPlayerFlush() has been called.
 * - VS_STATUS_EMPTY Player runs out of data. VsPlayerKick() will restart it.
 */
u_char VsGetStatus(void)
{
    return(vs_status);
}


/*!
 * \brief Initialize decoder memory test and return result.
 *
 * \return Memory test result.
 * - Bit(s) Mask            Meaning
            VS1011e/ VS1053
            VS1003
 * - 15     0x8000  0x8000  Test finished
 * - 14:7   Unused
 * - 14:10          Unused
 * - 9:             0x0200  Mux test succeeded
 * - 8:             0x0100  Good MAC RAM
 * - 7:             0x0080  Good I RAM
 * - 6:     0x0040          Mux test succeeded
 * - 6:             0x0040  Good Y RAM
 * - 5:     0x0020          Good I RAM
 * - 5:             0x0020  Good X RAM
 * - 4:     0x0010          Good Y RAM
 * - 4:             0x0010  Good I ROM 1
 * - 3:     0x0008          Good X RAM
 * - 3:             0x0008  Good I ROM 2
 * - 2:     0x0004          Good I ROM
 * - 2:             0x0004  Good Y ROM
 * - 1:     0x0002          Good Y ROM
 * - 1:             0x0002  Good X ROM 1
 * - 0:     0x0001          Good X ROM
 * - 0:             0x0001  Good X ROM 2
 *          0x807F  0x83FF  All ok
 *
 * - return 0 on succes; rc otherwise
 */
u_short VsMemoryTest(void)
{
    u_short rc = -1;
    u_char ief;
    static prog_char mtcmd[] = { 0x4D, 0xEA, 0x6D, 0x54, 0x00, 0x00, 0x00, 0x00};

    VsPlayerReset(0);
    VsPlayerSetMode(VS_SM_TESTS);

    ief = VsPlayerInterrupts(0);

    VsSdiWrite_P(mtcmd, sizeof(mtcmd));
    NutDelay(40);

    rc = VsRegRead(VS_HDAT0_REG);

    VsPlayerInterrupts(ief);

    if ((VsGetType()==VS_VS1003) && (rc == 0x807F))
    {
        rc=0;
    }

    return(rc);
}

/*!
 * \brief Set volume.
 *
 * \param left  Left channel volume.
 * \param right Right channel volume.
 *
 * \return 0 on success, -1 otherwise.
 */
int VsSetVolume(u_char left, u_char right)
{
    u_char ief;

    ief = VsPlayerInterrupts(0);

    VsRegWrite(VS_VOL_REG, (((u_short) left) << 8) | (u_short) right);

    VsPlayerInterrupts(ief);

    return(0);
}


/*!
 * \brief Get volume.
 *
 * \return u_short Volume.
 */
u_short VsGetVolume()
{
    u_char ief;
    u_short vol;
    ief = VsPlayerInterrupts(0);
    vol=VsRegRead(VS_VOL_REG);
    VsPlayerInterrupts(ief);
    return(vol);
}

/*!
 * \brief Return the number of the VS10xx chip.
 *
 * \return  The actual type number as defined
 */
u_short VsGetType(void)
{
    return(g_vs_type);
}


/*!
 * \brief Return the number of the VS10xx chip.
 *
 * \return  The actual type number coded in HEX!
 */
u_short VsGetTypeHex(void)
{
    switch (g_vs_type)
    {
        case VS_VS1003:  return(0x03);
        default:         break;         /* Other codes use the actual number. */
    }

    return(g_vs_type);
}

/*!
 * \brief Sine wave beep.
 *
 * \param fsin Frequency.
 * \param ms   Duration.
 *
 * \return 0 on success, -1 otherwise.
 */
int VsBeep(u_char fsin, u_short ms)
{
    u_char ief;
    static prog_char on[] = { 0x53, 0xEF, 0x6E};
    static prog_char off[] = { 0x45, 0x78, 0x69, 0x74};
    static prog_char end[] = { 0x00, 0x00, 0x00, 0x00};

    /* Disable decoder interrupts. */
    ief = VsPlayerInterrupts(0);

    VsPlayerSetMode(VS_SM_TESTS);

    fsin = (fsin* 16) + 56;
    VsSdiWrite_P(on, sizeof(on));
    VsSdiWrite(&fsin, 1);
    VsSdiWrite_P(end, sizeof(end));
    NutDelay(ms);
    VsSdiWrite_P(off, sizeof(off));
    VsSdiWrite_P(end, sizeof(end));

    /* Enable decoder interrupts. */
    VsPlayerInterrupts(ief);

    return(0);
}

/*!
 * \brief Sine wave beep start.
 *
 * \param fsin Frequency.
 *
 * \note this routine uses a fixed sample rate (12.000Hz) and
 *       therefor the possible frequencies are limited.
 *       Use 'VsBeepStartRaw' to generate a sinus with
 *       an arbitrary freqeuncy
 *
 * \return 0 on success, -1 otherwise.
 */
int VsBeepStart(u_char fsin)
{
    return(VsBeepStartRaw(56 + (fsin & 7) * 9));
}

/*!
 * \brief Sine wave beep start in Raw mode.
 *
 * \param Raw: b7..b5 determines samplerate
 *        while b4..b0 determines finepitch
 *
 * \note use the VS1003/VS1011 datasheet to figure out
 *       which value to use for Fs (b7..b5) and
 *       which value to use for S (b4..b0)
 *
 *
 * \return 0 on success, -1 otherwise.
 */
int VsBeepStartRaw(u_char Raw)
{
    u_char ief;
    static prog_char on[] = { 0x53, 0xEF, 0x6E};
    static prog_char end[] = { 0x00, 0x00, 0x00, 0x00};

    /* Disable decoder interrupts. */
    ief = VsPlayerInterrupts(0);

    VsPlayerSetMode(VS_SM_TESTS);

    VsSdiWrite_P(on, sizeof(on));
    VsSdiWrite(&Raw, 1);
    VsSdiWrite_P(end, sizeof(end));

    /* Enable decoder interrupts. */
    VsPlayerInterrupts(ief);

    return(0);
}

/*!
 * \brief Sine wave beep stop.
 *
 * \return 0 on success, -1 otherwise.
 */
int VsBeepStop()
{
    u_char ief;
    static prog_char off[] = { 0x45, 0x78, 0x69, 0x74};
    static prog_char end[] = { 0x00, 0x00, 0x00, 0x00};

    /* Disable decoder interrupts. */
    ief = VsPlayerInterrupts(0);

    VsSdiWrite_P(off, sizeof(off));
    VsSdiWrite_P(end, sizeof(end));

    /* Enable decoder interrupts. */
    VsPlayerInterrupts(ief);

    return(0);
}

static void VsLoadProgramCode(void)
{
    int i;

    for (i=0;i<CODE_SIZE;i++)
    {
        VsRegWrite(PRG_RDB(&atab[i]), PRG_RDW(&dtab[i]));
        // kick watchdog on a regular base
        if(i%500==0)
        {
            WatchDogRestart();
        }
    }
}
/*@}*/
