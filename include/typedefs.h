#ifndef _Typedefs_H
#define _Typedefs_H
/* ========================================================================
 * [PROJECT]    SIR
 * [MODULE]     global module
 * [TITLE]      system header file
 * [FILE]       typedefs.h
 * [VSN]        1.0
 * [CREATED]    09 november 2003
 * [LASTCHNGD]  09 november 2003
 * [COPYRIGHT]  Copyright (C) STREAMIT BV 2010
 * [PURPOSE]    global typedefs
 * ======================================================================== */


/*--------------------------------------------------------------------------*/
/*  Include files                                                           */
/*--------------------------------------------------------------------------*/
#include <fs/typedefs.h>

/*--------------------------------------------------------------------------*/
/*  Constant definitions                                                    */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Type declarations                                                       */
/*--------------------------------------------------------------------------*/
/* RL: this 'table' has now a mirror table in 'Display.c' (LcdErrorStrings) */
/*     Make sure that any modification made to this table are reflected by  */
/*     the LcdErrorStirngs table! (an error is bad but showing the wrong    */
/*     error is too much....) */
typedef enum _TERRORCODE
{
    OK = 0,                             /* All ok */

    /*
     * Status messages
     */
    CHANNEL_CONNECTING,                 /* Channel connecting */
    CHANNEL_RETRIEVING,                 /* Retrieving channels */
    STREAMER_CONNECTING,                /* Connecting to stream */
    STREAMER_BUFFERING,                 /* Buffering audio */
    STREAMER_PLAYING,                   /* Playing audio from a stream */
    STREAMER_FALLBACK,                  /* trying card now, inet failed*/
    UPDATE_CONNECTING,                  /* connecting to update server */
    CARD_BUFFERING,                     /* Buffering audio */
    CARD_PLAYING,                       /* Playing audio from a card */

    USER_ABORT,                         /* User abort */

    /*
     * Warnings. In other words, problems that are
     * probably recoverable by a retry
     */
    PLAYER_WARNINGS = 200,
    CHANNEL_HOSTNOTFOUND,               /* Could not resolve hostname */
    CHANNEL_NEW_ISP,                    /* New ISP settings received */
    CHANNEL_NEW_DB,                     /* New DB URL info received */
    CHANNEL_NEW_CHANNEL,                /* New channel info received */
    CHANNEL_NOCONNECT,                  /* Could not connect (will retry) */

    CHANNEL_TOO_MANY_NOCONNECTS,        /* Could not connect */
    CHANNEL_REDIRECT,                   /* Redirect (will retry) */
    CHANNEL_TOO_MANY_REDIRECTS,         /* Too many redirects */
    CHANNEL_BADRESPONSE,                /* Bad server response (will retry) */
    CHANNEL_TOO_MANY_BADRESPONSES,      /* Bad server response */

    STREAM_HOSTNOTFOUND,                /* Could not resolve hostname */
    STREAM_NOCONNECT,                   /* Could not connect (will retry) */
    STREAM_REDIRECT,                    /* Redirect (will retry) */
    STREAM_BADRESPONSE,                 /* Bad server response (will retry) */

    INET_HOSTNOTFOUND,                  /* Could not resolve hostname (will retry) */
    INET_NOCONNECT,                     /* Could not connect (will retry) */
    INET_BADRESPONSE,                   /* Bad server response (will retry) */
    INET_REDIRECT,                      /* Redirect (will retry) */
    INET_ACCESS_RESTRICTED,             /* Access needs login info (will retry) */

    STREAM_TIMEOUT,                     /* Network timeout */
    STREAM_BADAUDIO,                    /* Audio data seems invalid */
    STREAM_DISCONNECTED,                /* Server closed the connection */
    STREAM_BUFFEREMPTY,                 /* Audio buffer ran out of data */
    STREAM_BADCHANNEL,                  /* Bad channel number */

    UPDATE_NEEDED,                      /* Firmware update is needed */

    PLAYER_WAITPLAY,                    /* Player has nothing to do */
    PLAYER_STARTING,

    CARD_BUFFEREMPTY,                   /* Audio buffer ran out of data */
    CARD_BADPLAYLIST,                   /* non-existing or invalid playlist requested */
    CARD_BADAUDIO,                      /* Audio data seems invalid */

    /*
     * Errors.
     * User interaction (e.g. select a different channel) can solve these
     */
    PLAYER_ERRORS = 400,
    BADCHANNEL,                         /* Bad channel number                   - no longer used */
    STREAM_TOO_MANY_NOCONNECTS,         /* Too many no connects */
    STREAM_TOO_MANY_BADRESPONSES,       /* Too many bad server responses */
    STREAM_TOO_MANY_REDIRECTS,          /* Too many redirects */
    UPDATE_FAILED,                      /* not specified, just failed */

    CHANNEL_NODATA,                     /* Didn't receive any data */

    STREAM_BAD_FILETYPE,                /* Bad type of file */
    STREAM_BAD_NETWORK,                 /* Too many network failures            - no longer used */
    STREAM_DISCONNECTED_UNUSED,         /* Server closed the connection         - no longer used */
    STREAM_BUFFEREMPTY_UNUSED,          /* Audio buffer ran out of data         - no longer used */

    INET_HOST_NONEXISTANT,              /* Hostname unknown */
    INET_TOO_MANY_NOCONNECTS,           /* Too many no connects */
    INET_TOO_MANY_REDIRECTS,            /* Too many redirects */
    INET_TOO_MANY_BADRESPONSES,         /* Too many bad server responses */
    INET_ACCESS_DENIED,                 /* Access to the server was denied */

    CARD_NO_SONG,                       /* no (more) songs found on this card */
    CARD_PLAYLIST_IN_USE,               /* trying to open a playlist that was open already */
    CARD_CREATE_STREAM,                 /* Failed to create a stream */
    CARD_NO_CARD,                       /* card not present to perfom desired action */
    CARD_NO_HEAP,                       /* unable to allocate RAM */
    CARD_NOT_REGISTERED,                /* card present but not know in the system */
    CARD_WRONG_HASH,                    /* hash results in a non-valid flash-address */
    /*
     * System Errors.
     * These include programming errors but also:
     * errors that may (or may not..) be solved by a reboot (indicated by REBOOT)
     */
    PLAYER_SYSTEMERRORS = 500,
    PLAYER_NOTREADY,                    /* Player was not successfully initialised */
    SESSION_NODEVICE,                   /* Could not register devices */
    SESSION_NODHCP_NOEEPROM,            /* No DHCP and no previous IP address. REBOOT */
    SESSION_MDMNOINIT,                  /* Could not initialise modem. REBOOT */
    SESSION_MDMNODISCONNECT,            /* Modem is still connected. REBOOT */

    SESSION_PPPINIT,                    /* Could not initialise PPP */
    SESSION_PPPSTART,                   /* Could not start PPP (username/password incorrect?)
                                           Note that NutOs ALWAYS requires a REBOOT in this case! */
    SESSION_NOROUTEADD,                 /* Could not add route to routetable */
    CHANNEL_NOMEM,                      /* Not enough memory for channel */
    CHANNEL_CREATE_SOCKET,              /* Failed to create a socket */

    CHANNEL_SOCK_RCVTO,                 /* Could not set socket option RCVTO */
    CHANNEL_CREATE_STREAM,              /* Failed to create a stream */
    STREAM_NOTHREAD,                    /* Could not start streamer thread */
    STREAM_NOMEM,                       /* Not enough memory for streamer */
    STREAM_CREATE_SOCKET,               /* Failed to create a socket */

    STREAM_CREATE_STREAM,               /* Failed to create a stream */
    STREAM_SOCK_MSS,                    /* Could not set socket option MSS */
    STREAM_SOCK_RCVTO,                  /* Could not set socket option RCVTO */
    STREAM_SOCK_RXBUF,                  /* Could not set socket option RXBUF */

    UPDATE_WRONG_NROF_BYTES,            /* nrof bytes not as specified in 'update.inf' */
    UPDATE_CODESIZE_OVERFLOW,           /* codesize exceeds 126KB (note: 2KB needed for bootloader) */
    UPDATE_CRC_ERROR,                   /* CRC of downloaded code in RAM incorrect */
    UPDATE_NOT_ALLOWED,                 /* either ISP or bootloader prohibits Remote Update functionality */

    INET_NOMEM,                         /* Not enough memory */
    INET_CREATE_SOCKET,                 /* Failed to create a socket */
    INET_SOCK_MSS,                      /* Could not set socket option MSS */
    INET_SOCK_RCVTO,                    /* Could not set socket option RCVTO */
    INET_SOCK_RXBUF,                    /* Could not set socket option RXBUF */
    INET_CREATE_STREAM,                 /* Failed to create a stream */
    INET_SEND_FAIL,                     /* Failed to send data */

    STREAM_TOO_MANY_ERRORS,             /* Seen too many errors */

    PLAYER_NO_THREAD,                   /* Could not start player thread */
    PLAYER_NO_SOURCE,                   /* No source to play audio from */
} TError;

typedef void (*thead_start_t)(void*);

/*--------------------------------------------------------------------------*/
/*  Global variables                                                        */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Global functions                                                        */
/*--------------------------------------------------------------------------*/

#endif /* _Typedefs_H */
