#ifndef _CFG_ARCH_H_
#define _CFG_ARCH_H_

/*
 * Copyright (C) 2004-2006 by egnite Software GmbH. All rights reserved.
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
 *
 */

/*
 * This file is reserved to specify architecture dependant
 * configuration and is currently used as a placeholder 
 * when not using the Configurator.
 */

/*
** Determine board defaults.
**/
#if defined(ETHERNUT1)

#ifndef PLATFORM
#define PLATFORM ETHERNUT1
#endif

#ifndef AVR_GCC
#define AVR_GCC
#endif

#if !defined(MCU_ATMEGA128) && !defined(MCU_ATMEGA2560) && !defined(MCU_ATMEGA2561)
#if defined(__AVR_ATmega2561__) || defined(ATMega2561)
#define MCU_ATMEGA2561
#elif defined(__AVR_ATmega2561__) || defined(ATMega2561)
#define MCU_ATMEGA2560
#else
#define MCU_ATMEGA128
#endif
#endif

#elif defined(ETHERNUT2)

#ifndef PLATFORM
#define PLATFORM ETHERNUT2
#endif

#ifndef AVR_GCC
#define AVR_GCC
#endif

#if !defined(MCU_ATMEGA128) && !defined(MCU_ATMEGA2560) && !defined(MCU_ATMEGA2561)
#if defined(__AVR_ATmega2561__) || defined(ATMega2561)
#define MCU_ATMEGA2561
#elif defined(__AVR_ATmega2561__) || defined(ATMega2561)
#define MCU_ATMEGA2560
#else
#define MCU_ATMEGA128
#endif
#endif

#elif defined(ETHERNUT3)

#ifndef PLATFORM
#define PLATFORM ETHERNUT3
#endif

#ifndef ARM_GCC
#define ARM_GCC
#endif

#ifndef MCU_AT91R40008
#define MCU_AT91R40008
#endif

#elif defined(ETHERNUT5)

#ifndef PLATFORM
#define PLATFORM ETHERNUT5
#endif

#ifndef ARM_GCC
#define ARM_GCC
#endif

#ifndef MCU_AT91SAM9XE512
#define MCU_AT91SAM9XE512
#endif

#elif defined(AT91SAM7X_EK)

#ifndef PLATFORM
#define PLATFORM AT91SAM7X_EK
#endif

#ifndef ARM_GCC
#define ARM_GCC
#endif

#ifndef MCU_AT91SAM7X256
#define MCU_AT91SAM7X256
#endif

#elif defined(AT91SAM7S)

#ifndef PLATFORM
#define PLATFORM AT91SAM7S
#endif

#ifndef ARM_GCC
#define ARM_GCC
#endif

#ifndef MCU_AT91SAM7S256
#define MCU_AT91SAM7S256
#endif

#elif defined(AT91SAM7SE_EK)

#ifndef PLATFORM
#define PLATFORM AT91SAM7SE_EK
#endif

#ifndef ARM_GCC
#define ARM_GCC
#endif

#ifndef MCU_AT91SAM7SE512
#define MCU_AT91SAM7SE512
#endif

#elif defined(ELEKTOR_IR1)

#ifndef PLATFORM
#define PLATFORM ELEKTOR_IR1
#endif

#ifndef ARM_GCC
#define ARM_GCC
#endif

#ifndef MCU_AT91SAM7SE512
#define MCU_AT91SAM7SE512
#endif

#elif defined(AT91SAM9260_EK)

#ifndef PLATFORM
#define PLATFORM AT91SAM9260_EK
#endif

#ifndef ARM_GCC
#define ARM_GCC
#endif

#ifndef MCU_AT91SAM9260
#define MCU_AT91SAM9260
#endif

#elif defined(AT91SAM9G45_EK)

#ifndef PLATFORM
#define PLATFORM AT91SAM9G45_EK
#endif

#ifndef ARM_GCC
#define ARM_GCC
#endif

#ifndef MCU_AT91SAM9G45
#define MCU_AT91SAM9G45
#endif

#endif /* Board */

/*
** Determine Atmel MCU family.
**/
#if defined(MCU_AT91SAM7S16) || defined(MCU_AT91SAM7S32) || defined(MCU_AT91SAM7S64) \
    || defined(MCU_AT91SAM7S128) || defined(MCU_AT91SAM7S256) || defined(MCU_AT91SAM7S512)
#ifndef MCU_AT91SAM7S
#define MCU_AT91SAM7S
#endif
#endif

#if defined(MCU_AT91SAM7SE32) || defined(MCU_AT91SAM7SE256) || defined(MCU_AT91SAM7SE512)
#ifndef MCU_AT91SAM7SE
#define MCU_AT91SAM7SE
#endif
#endif

#if defined(MCU_AT91SAM7X128) || defined(MCU_AT91SAM7X256) || defined(MCU_AT91SAM7X512)
#ifndef MCU_AT91SAM7X
#define MCU_AT91SAM7X
#endif
#endif

#if defined(MCU_AT91SAM9XE128) || defined(MCU_AT91SAM9XE256) || defined(MCU_AT91SAM9XE512)
#ifndef MCU_AT91SAM9XE
#define MCU_AT91SAM9XE
#endif
#endif

#if defined(MCU_AT91SAM7S) || defined(MCU_AT91SAM7SE) || defined(MCU_AT91SAM9XE) || defined(MCU_AT91SAM9G45)
#ifndef MCU_AT91
#define MCU_AT91
#endif
#endif

#endif