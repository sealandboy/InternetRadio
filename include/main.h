#ifndef _HTTPSERV_H_
#define _HTTPSERV_H_
/*
 * Copyright (C) 2001-2007 by egnite Software GmbH. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 */

/*!
 * $Log$
 */

#include <dev/board.h>
#include <cfg/os.h>

#define ETH0_BASE	0xC300
#define ETH0_IRQ	5

/* Wether we should use DHCP. */
#define USE_DHCP

/* Wether we should run a discovery responder. */
#define USE_DISCOVERY

/* Wether to use PHAT file system. UROM is default. */
// #define USE_PHAT

/* Wether to use date and time functions. */
#define USE_DATE_AND_TIME

/* Wether to use CGI parameters. */
#define USE_CGI_PARAMETERS

/* Wether to use SSI. */
#define USE_SSI

/* Wether to use ASP. */
#define USE_ASP

/* Wether to use dynamically created threads. */
#define USE_DYNAMIC_THREADS

/* REMCO - overige defines */
// #define MY_BLKDEV
#define MY_HTTPROOT
#define USE_CGI_PARAMETERS
#define USE_SSI
#define USE_ASP
#define MY_FSDEV_NAME



/* 
 * Unique MAC address of the target Board. 
 *
 * Ignored if non-volatile memory contains a valid configuration.
 */
#define MY_MAC  "\x00\x06\x98\x30\x00\x35"

/* 
 * Unique IP address of the Ethernut Board. 
 *
 * Ignored if DHCP is used. 
 */
#define MY_IPADDR "192.168.192.35"

/* 
 * IP network mask of the Ethernut Board.
 *
 * Ignored if DHCP is used. 
 */
#define MY_IPMASK "255.255.255.0"

/* 
 * Gateway IP address for the Ethernut Board.
 *
 * Ignored if DHCP is used. 
 */
#define MY_IPGATE "192.168.192.1"

/*! \brief Local timezone, -1 for Central Europe. */
#ifdef USE_DATE_AND_TIME
#define MYTZ    -1
#endif

/*! \brief IP address of the host running a time daemon. */
#ifdef USE_DATE_AND_TIME
#define MYTIMED "130.149.17.21"
#endif

/* Verbose debug port. */
//#define HTTPD_VERBOSE

/* Number of server threads always running. */
#ifndef HTTPD_MIN_THREADS
#define HTTPD_MIN_THREADS   4
#endif

/* Maximum number of server threads. */
#if defined(USE_DYNAMIC_THREADS) && !defined(HTTPD_MAX_THREADS)
#define HTTPD_MAX_THREADS   16
#endif

/* Server thread stack size. */
#ifndef HTTPD_SERVICE_STACK
#define HTTPD_SERVICE_STACK NUT_THREAD_MAINSTACK
#endif

/* The TCP port we are listening at. */
#ifndef HTTPD_TCP_PORT
#define HTTPD_TCP_PORT      80
#endif

/*
 * Maximum segment size. 
 *
 * Choose 536 up to 1460. Note, that segment sizes above 536 may result 
 * in fragmented packets, which are not supported by Nut/Net.
 */
#ifndef HTTPD_MAX_SEGSIZE
#define HTTPD_MAX_SEGSIZE   1460
#endif

/* TCP stream buffer size. Rule of thumb: 6 times MSS. */
#ifndef HTTPD_TCP_BUFSIZE
#define HTTPD_TCP_BUFSIZE   8760
#endif

/* Closes keep-alive connections. */
#ifndef HTTPD_TCP_TIMEOUT
#define HTTPD_TCP_TIMEOUT   500
#endif

#ifdef USE_PHAT
/* MMC device drivers used by different boards. */
//remco #if defined(ETHERNUT3)
#define MY_BLKDEV      devNplMmc0
#elif defined(AT91SAM7X_EK)
#define MY_BLKDEV      devAt91SpiMmc0
#elif defined(AT91SAM9260_EK)
#define MY_BLKDEV      devAt91Mci0
#define MY_BLKDEV_NAME "MCI0"
//remco #endif

/* Name of the MMC device driver. */
#ifndef MY_BLKDEV_NAME
#define MY_BLKDEV_NAME "MMC0" 
#endif

/* File system driver. */
#ifndef MY_FSDEV
#define MY_FSDEV       devPhat0
#endif

/* File system driver name. */
#ifndef MY_FSDEV_NAME
#define MY_FSDEV_NAME  "PHAT0" 
#endif

#endif /* USE_PHAT */

/* By default UROM is used. */
#ifndef MY_FSDEV
#define MY_FSDEV        devUrom
#endif

#ifdef MY_FSDEV_NAME
#define MY_HTTPROOT     MY_FSDEV_NAME ":/html/" 
#endif

/* ICCAVR Demo is limited. Try to use the bare minimum. */
#if defined(__IMAGECRAFT__)
#undef USE_DHCP
#undef USE_DISCOVERY
#undef USE_PHAT
#undef USE_DATE_AND_TIME
#undef USE_CGI_PARAMETERS
#undef USE_SSI
#undef USE_ASP
#undef USE_DYNAMIC_THREADS
#endif /* __IMAGECRAFT__ */

#endif
