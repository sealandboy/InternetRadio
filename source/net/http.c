/*
 *  Copyright STREAMIT BV, 2010.
 *
 *  Project             : SIR
 *  Module              : Http
 *  File name  $Workfile: Http.c  $
 *       Last Save $Date: 2003/08/23 18:39:38  $
 *             $Revision: 0.1  $
 *  Creation Date       : 2003/08/23 18:39:38
 *
 *  Description         : Http client routines
 *
 */

#define LOG_MODULE  LOG_HTTP_MODULE

/*--------------------------------------------------------------------------*/
/*  Include files                                                           */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/confos.h>
#include <arpa/inet.h>
#include <netdb.h>

//#pragma text:appcode

#include "system.h"
#include "log.h"
#include "settings.h"
#include "util.h"

#include "http.h"

/*--------------------------------------------------------------------------*/
/*  Constant definitions                                                    */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Type declarations                                                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Local variables                                                         */
/*--------------------------------------------------------------------------*/
static CONST char EncTable[] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

/*--------------------------------------------------------------------------*/
/*  Global variables                                                        */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Local functions                                                         */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Global functions                                                        */
/*--------------------------------------------------------------------------*/

/*!
 * \brief Calculate the space needed to store data in Base64
 *
 * \param NrOfBytes     number of bytes to encode
 *
 * \return Number of bytes needed
 */
int Base64EncodedSize(size_t tNrOfBytes)
{
    return ((tNrOfBytes + 2) / 3 * 4) + 1;
}

/*!
 * \brief Encode (binary) data into a Base64 encoded string.
 *
 * \param szDest        pointer to destination
 * \param pSrc          pointer to (binary) data to encode.
 * \param tSize         nrof bytes to read from pSrc
 *
 * \return Number of bytes copied to szDest
 *         0 if errors or nothing to encode
 */
size_t Base64Encode(char *szDest, CONST u_char *pSrc, size_t tSize)
{
    size_t DestLen = 0;                 /* nrof bytes in szDest */
    u_char Index = 0;
    unsigned char Tmp[3];               /* source data */

    /* Encode all input data */
    while (tSize > 0)
    {
        /* Get a piece of data to encode */
        memset(Tmp, 0, sizeof(Tmp));
        for (Index = 0; Index < sizeof(Tmp) && tSize > 0; Index++, tSize--)
        {
            Tmp[Index] = *pSrc++;
        }

        /* Encode 3 chars into 4 */
        *szDest++ =                     EncTable[                          ((Tmp[0] >> 2) & 0x3F) ];
        *szDest++ =                     EncTable[ ((Tmp[0] << 4) & 0x3F) | ((Tmp[1] >> 4) & 0x0F) ];
        *szDest++ = (Index < 1) ? '=' : EncTable[ ((Tmp[1] << 2) & 0x3C) | ((Tmp[2] >> 6) & 0x03) ];
        *szDest++ = (Index < 2) ? '=' : EncTable[ ( Tmp[2]       & 0x3F) ];
        DestLen += 4;
    }

    *szDest = '\0';
    return (DestLen);
}

/*!
 * \brief Get ip address of a host
 *
 * \param szHostName Name or string of the IP address
 *                   of the host
 * \return The IP address of the host.
 *         0 if not an IP address, or we could not resolve the name
 */
u_long GetHostByName(CONST char *szHostName)
{
    u_long dwAddress;

    if ((dwAddress = inet_addr(szHostName)) == (u_long)-1)
    {
        dwAddress = NutDnsGetHostByName((u_char*)szHostName);
    }
    return (dwAddress);
}

/*!
 * \brief Break a Url down in parts
 *
 * The hostname, port and URI pointers are
 * set to the appropriate locations or an empty string
 * if not present.
 *
 * \note szUrl is modified
 *
 * \param szUrl Url to parse
 */
void HttpParseUrl(char *szUrl, TUrlParts *tUrlParts)
{
    char *szStart;      /* Points to the first character of the part */
    char *szEnd;        /* Points to the last character of the part */

    /*
     * In case we don't find a Host, port or URI, point
     * to empty string
     */
    tUrlParts->pszHost = tUrlParts->pszPort = tUrlParts->pszPath = (char *)(szUrl + strlen(szUrl));

    /*
     * skip the prefix
     */
    szStart = strstr_P(szUrl, PSTR("://"));
    if (szStart != NULL)
    {
        szStart += 3;
    }
    else
    {
        /*
         * Apparently there is no prefix
         */
        szStart  = (char *)szUrl;
    }

    /*
     * We have found the hostname
     */
    tUrlParts->pszHost = szStart;

    /*
     * Find the end of the hostname
     * End of it is indicated by ':' or '/'
     * If neither are found, assume we have a URL in
     * the form 'http://demeterkast.net'
     */
    szEnd = strchr(szStart, ':');
    if (szEnd != NULL)
    {
        /*
         * There is a port specification, get it now
         */
        *szEnd = '\0';          /* Terminate the previous part */
        szStart = szEnd + 1;        /* point to the portnumber */
        tUrlParts->pszPort = szStart;
    }

    szEnd = strchr(szStart, '/');
    if (szEnd != NULL)
    {
        /*
         * There is a URI specification, get it now
         */
        *szEnd = '\0';          /* Terminate the previous part */
        tUrlParts->pszPath = szEnd + 1;   /* point to the URI */
    }
}

/*!
 * \brief Send a request to a server.
 *
 * The connection to the server should already be
 * present and associated with a stream.
 *
 * \param   ptStream [in] Opened stream to send the request to
 * \param   pszHeaders [in] The headers to send
 * \param   wMode [in] Bitmask for the request to send
 *              - HTTP_AUTH to send a Basic authentication consisting of our hostname and an empty password
 * \return  The number of characters written or a negative value to
 *          indicate an error.
 */
int HttpSendRequest(FILE *ptStream, CONST char *pszHeaders, u_short wMode)
{
    int nResult = 0;    /* Bytes sent during last call (or -1) */
    int nSent = 0;      /* Total bytes sent */

    if ((pszHeaders == NULL) || (ptStream == NULL))
    {
        nResult = -1;
    }

    /*
     * Send the headers
     */
    if (nResult >= 0)
    {
        nSent += nResult;
        nResult = fprintf_P(ptStream, PSTR("%s"), pszHeaders);
    }

    /*
     * Add authentication info (if requested)
     */
    if (nResult >= 0)
    {
        nSent += nResult;
        nResult = 0;

        if (wMode & HTTP_AUTH)
        {
            char *szEncoded;
            char szToken[sizeof(confos.hostname)+1];

            /*
             * Compose the username:password
             * (We use our hostname and an empty password)
             */
            strncpy(szToken, confos.hostname, sizeof(szToken)-1);
            szToken[sizeof(szToken)-1] = '\0';
            strcat_P(szToken, PSTR(":"));

            szEncoded = MyMalloc(Base64EncodedSize(strlen(szToken)));
            if (szEncoded == NULL)
            {
                nResult = -1;
            }
            else
            {
                (void)Base64Encode(szEncoded, (u_char*)szToken, strlen(szToken));

                nResult = fprintf_P(ptStream, PSTR("Authorization: Basic %s\r\n"), szEncoded);

                MyFree(szEncoded);
            }
        }
    }

    /*
     * Print the end of header
     */
    if (nResult >= 0)
    {
        nSent += nResult;
        nResult = fprintf_P(ptStream, PSTR("\r\n"));
    }

    fflush(ptStream);

    if (nResult >= 0)
    {
        nSent += nResult;
    }
    else
    {
        nSent = -1;
    }

    return (nSent);
}

