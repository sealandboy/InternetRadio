/*
 *  Copyright STREAMIT BV, 2010.
 *
 *  Project             : SIR
 *  Module              : Inet
 *  File name  $Workfile: Inet.c  $
 *       Last Save $Date: 2006/02/24 13:46:16  $
 *             $Revision: 0.1  $
 *  Creation Date       : 2006/02/24 13:46:16
 *
 *  Description         :
 *
 */

#define LOG_MODULE  LOG_INET_MODULE

/*--------------------------------------------------------------------------*/
/*  Include files                                                           */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/timer.h>
#include <sys/socket.h>
#include <sys/heap.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>

//#pragma text:appcode

#include "system.h"
#include "version.h"
#include "log.h"
#include "settings.h"
#include "util.h"

#include "inet.h"

/*--------------------------------------------------------------------------*/
/*  Constant definitions                                                    */
/*--------------------------------------------------------------------------*/
/*!\brief Allow some problems before giving up */
//#define MAX_NODNS               5
#define MAX_NODNS               2
#define MAX_NOCONNECT           2
#define MAX_BADRESPONSE         5
#define MAX_REDIRECT            5   /* see RFC2616 */

/*!\brief Default Receive timeout */
#define TCP_RECVTO_DEFAULT      5000

/*!\brief HTTP line buffer size. Allocates in chunks of this size. */
#define HTTP_HEADER_LINE_SIZE   512

#ifdef DEBUG
//#define INET_DEBUG
#endif /* #ifdef DEBUG */

/*--------------------------------------------------------------------------*/
/*  Type declarations                                                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Local variables                                                         */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Global variables                                                        */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Local functions                                                         */
/*--------------------------------------------------------------------------*/
static TError Connect(HINET hInet);
static void GetHeaders(HINET hInet);
static int CreateRequest(HINET hInet, CONST char *pszMethod, CONST char *pszPath, CONST char *pszAccept);
static void CloseDescriptors(HINET hInet);

#ifdef INET_DEBUG
static void ShowDebug(void)
{
    LogMsg_P(LOG_DEBUG, PSTR("free %d"), NutHeapAvailable());
}
#else
#define ShowDebug()
#endif

/*!
 * \brief Opens an Internet session.
 *
 * \param   hInet [in] Handle returned by a previous call to InternetOpen.
 *
 * \return  0 if the connection is successful
 *          TError otherwise.
 */
static TError Connect(HINET hInet)
{
    TError tError = OK;
    unsigned char byDone = 0;
    char ModeString[5];

    /*
     * Connect to the server. Retry in case of problems
     */
    while (byDone == 0)
    {
        if (hInet->tState == INET_STATE_CLOSING)
        {
            tError = USER_ABORT;
        }

        if (tError == OK)
        {
            hInet->tState = INET_STATE_BUSY;
        }

        /*
         * Translate to an IP number and port
         */
        if (tError == OK)
        {
            if ((hInet->wPort = atoi(hInet->tUrlParts.pszPort)) == 0)
            {
                // Use defaults if not specified
                hInet->wPort = 80;
            }
            LogMsg_P(LOG_DEBUG, PSTR("Looking up [%s]"), hInet->tUrlParts.pszHost);

            if ((hInet->ulIpAddress = GetHostByName(hInet->tUrlParts.pszHost)) == 0)
            {
                tError = INET_HOSTNOTFOUND;

                /*
                 * Check if we can retry
                 */
                if (++hInet->tRetries.byNoDnsCount >= MAX_NODNS)
                {
                    /* Too many failures, stop */
                    tError = INET_HOST_NONEXISTANT;
                }
            }
        }

        /*
         * We could have been asleep; Check if we have received a close request
         */
        if (hInet->tState == INET_STATE_CLOSING)
        {
            tError = USER_ABORT;
        }

        /*
         * Create a socket.
         */
        ShowDebug();
        if (tError == OK)
        {
            if ((hInet->ptSocket = NutTcpCreateSocket()) == 0)
            {
                tError = INET_CREATE_SOCKET;
            }
        }
#ifdef INET_DEBUG
        LogMsg_P(LOG_DEBUG, PSTR("ptSocket @%X"), hInet->ptSocket);
#endif /* #ifdef INET_DEBUG */
        ShowDebug();

        /*
         * Set socket options
         */
        if (tError == OK)
        {
            /* We use our own default if not specified */
            if (hInet->ulRecvTimeout == 0)
            {
                hInet->ulRecvTimeout = TCP_RECVTO_DEFAULT;
            }
            /* Nut/OS defaults to infinite receive timeout. So always set our default timeout */
            if (NutTcpSetSockOpt(hInet->ptSocket, SO_RCVTIMEO, &hInet->ulRecvTimeout, sizeof(hInet->ulRecvTimeout)))
            {
                tError = INET_SOCK_RCVTO;
            }
        }
        if (tError == OK)
        {
            /* Use NutOS's default if not specified */
            if ((hInet->unMss != 0) &&
                (NutTcpSetSockOpt(hInet->ptSocket, TCP_MAXSEG, &hInet->unMss, sizeof(hInet->unMss))))
            {
                tError = INET_SOCK_MSS;
            }
        }
        if (tError == OK)
        {
            /* Use NutOS's default if not specified */
            if ((hInet->unTcpRecvBufSize != 0) &&
                (NutTcpSetSockOpt(hInet->ptSocket, SO_RCVBUF, &hInet->unTcpRecvBufSize, sizeof(hInet->unTcpRecvBufSize))))
            {
                tError = INET_SOCK_RXBUF;
            }
        }

        /*
         * Connect to destination
         */
        if (tError == OK)
        {
            LogMsg_P(LOG_DEBUG, PSTR("Connecting to %s:%d"), inet_ntoa(hInet->ulIpAddress), hInet->wPort);
            if (NutTcpConnect(hInet->ptSocket, hInet->ulIpAddress, hInet->wPort) != 0)
            {
                tError = INET_NOCONNECT;

                LogMsg_P(LOG_ERR, PSTR("No connect"));

                if (++hInet->tRetries.byNoConnectCount >= MAX_NOCONNECT)
                {
                    tError = INET_TOO_MANY_NOCONNECTS;
                }
            }
            else
            {
                /* Connected, stop */
                byDone = 1;

                LogMsg_P(LOG_DEBUG, PSTR("TCP Connected"));
                /* Let the TCP/IP stack settle down first */
                NutSleep(500);
            }
        }

        /*
         * We could have been asleep; Check if we have receive a close request
         */
        if (hInet->tState == INET_STATE_CLOSING)
        {
            tError = USER_ABORT;
        }

        /*
         * Create a stream from the socket.
         */
        if (tError == OK)
        {
            strcpy_P(ModeString, PSTR("r+b"));
            if ((hInet->ptStream = _fdopen((int)hInet->ptSocket, ModeString)) == 0)
            {
                LogMsg_P(LOG_ERR, PSTR("No stream %d"), errno);
                tError = INET_CREATE_STREAM;
            }
        }

#ifdef INET_DEBUG
        LogMsg_P(LOG_DEBUG, PSTR("ptStream @%X"), hInet->ptStream);
#endif /* #ifdef INET_DEBUG */


        if (tError != OK)
        {
            LogMsg_P(LOG_ERR, PSTR("Error [%d]"), tError);

            /*
             * Check if we need to try again
             */
            if ((tError > PLAYER_WARNINGS) && (tError < PLAYER_ERRORS))
            {
                LogMsg_P(LOG_INFO, PSTR("Retry"));

                CloseDescriptors(hInet);

                /*
                 * Try again
                 */
                tError = OK;

                /*
                 * Give other threads some time before
                 * we try again
                 */
                NutSleep(300);
            }
            else
            {
                /* Errors, stop */
                byDone = 1;
            }
        }
    } /* end while */

    ShowDebug();

    hInet->tState = INET_STATE_IDLE;
    return (tError);
}

/*!
 * \brief Create a new request to be sent to an Internet server.
 *
 * hInet->hRequest should already have allocated memory behind it, as this
 * routine does not allocate (or free) memory.
 *
 * \param   hInet [in] Handle returned by a previous call to InternetOpen.
 * \param   pszMethod [in] A pointer to a null-terminated string that contains
 *          the method to use in the request. If this parameter is NULL, the
 *          function uses GET.
 * \param   pszPath [in] A pointer to a null-terminated string that contains
 *          the path to act upon.
 * \param   pszAccept[in] A pointer to a null-terminated string that indicates
 *          the media types accepted by the client. If this parameter is NULL,
 *          a string that indicates that all types are accepted is sent to the
 *          server.
 *
 * \return  0 when the request was successfully created
 *          -1 on errors
 */
static int CreateRequest(HINET hInet, CONST char *pszMethod, CONST char *pszPath, CONST char *pszAccept)
{
    static prog_char cszAction_P[]      = "%s /%s%s HTTP/1.0\r\n";
    static prog_char cszUserAgent_P[]   = "User-Agent: %s/%s s/n:%s\r\n";

    // create buffers for RAM-strings...
    static char szEmptyString[1];
    static char szGet[4];
    static char szSerialNum[9];
    static char szAcceptDefault[20];

    int nResult = 0;
    char *pszSerialNr;

    // apply defaults for RAM-strings....
    szEmptyString[0] = '\0';
    szSerialNum[0] = '\0';
    strcpy_P(szGet, PSTR("GET"));
    strcpy_P(szAcceptDefault, PSTR("Accept: */*\r\n"));

    pszSerialNr = szEmptyString;

#ifdef INET_DEBUG
    LogMsg_P(LOG_DEBUG, PSTR("Create request"));
#endif /* #ifdef INET_DEBUG */

    if ((hInet == NULL) || (hInet->hRequest == NULL))
    {
        /* Bad argument */
        nResult = -1;
    }

    if (nResult >= 0)
    {
        /*
         * Erase any previous requests
         */
        hInet->hRequest->unRequestInUse = 0;
        hInet->hRequest->pszRequest[0] = '\0';

        /*
         * Use defaults for the method and Uri if not further specified
         */
        if (pszMethod == NULL)
        {
            pszMethod = szGet;
        }
        if (pszPath == NULL)
        {
            pszPath = hInet->tUrlParts.pszPath;
        }

        //sprintf_P(szSerialNum, PSTR("%5.5lX"), SettingsGetSerialnumber());
    }

    /*
     * Check if we need to add our serial number to the end of the URL
     */
    if (nResult >= 0)
    {
        if ((hInet->hRequest->wOptions & INET_FLAG_ADD_SERIAL) == INET_FLAG_ADD_SERIAL)
        {
            if (strlen(pszPath) > 0)
            {
                if (pszPath[strlen(pszPath)-1] == '=')
                {
                    pszSerialNr = szSerialNum;
                }
            }
        }
    }

    /*
     * Create the request
     */
    if (nResult >= 0)
    {
        nResult = sprintf_P(&hInet->hRequest->pszRequest[hInet->hRequest->unRequestInUse],
                            cszAction_P,
                            pszMethod,
                            pszPath,
                            pszSerialNr);
        if (nResult >= 0)
        {
#ifdef INET_DEBUG
            /* We already print out the request at the end of this routine */
#else /* #ifdef INET_DEBUG */
//            LogMsg_P(LOG_DEBUG, PSTR("Request [%s]"), &hInet->hRequest->pszRequest[hInet->hRequest->unRequestInUse]);
#endif /* #ifdef INET_DEBUG */
            hInet->hRequest->unRequestInUse += nResult;
            if (hInet->hRequest->unRequestInUse > hInet->hRequest->unRequestBufSize)
            {
                nResult = -1;
            }
        }
    }

    /*
     * Add the User-Agent
     */
    if (nResult >= 0)
    {
        //nResult = sprintf_P(&hInet->hRequest->pszRequest[hInet->hRequest->unRequestInUse],
        //                    cszUserAgent_P,
                            //VersionGetAppProductName(),
                            //VersionGetAppString(),
        //                    szSerialNum);
        if (nResult >= 0)
        {
            hInet->hRequest->unRequestInUse += nResult;
            if (hInet->hRequest->unRequestInUse > hInet->hRequest->unRequestBufSize)
            {
                nResult = -1;
            }
        }
    }

    /*
     * Add the host header if needed
     */
    if (nResult >= 0)
    {
        if ((hInet->tUrlParts.pszHost != NULL) && (strlen(hInet->tUrlParts.pszHost) > 0))
        {
            if ((hInet->tUrlParts.pszPort != NULL) && (strlen(hInet->tUrlParts.pszPort) > 0))
            {
                nResult = sprintf_P(&hInet->hRequest->pszRequest[hInet->hRequest->unRequestInUse],
                                    PSTR("Host: %s:%s\r\n"),
                                    hInet->tUrlParts.pszHost,
                                    hInet->tUrlParts.pszPort);
            }
            else
            {
                nResult = sprintf_P(&hInet->hRequest->pszRequest[hInet->hRequest->unRequestInUse],
                                    PSTR("Host: %s\r\n"),
                                    hInet->tUrlParts.pszHost);
            }
            if (nResult >= 0)
            {
                hInet->hRequest->unRequestInUse += nResult;
                if (hInet->hRequest->unRequestInUse > hInet->hRequest->unRequestBufSize)
                {
                    nResult = -1;
                }
            }
        }
    }

    /*
     * Add the accept header
     */
    if (nResult >= 0)
    {
        if (pszAccept == NULL)
        {
            pszAccept = szAcceptDefault;
        }
        nResult = sprintf(&hInet->hRequest->pszRequest[hInet->hRequest->unRequestInUse],
                          szAcceptDefault,
                          pszAccept);

        if (nResult >= 0)
        {
            hInet->hRequest->unRequestInUse += nResult;
            if (hInet->hRequest->unRequestInUse > hInet->hRequest->unRequestBufSize)
            {
                nResult = -1;
            }
        }
    }

    /*
     * Check if we need to do a request for ICY meta data
     */
    if (nResult >= 0)
    {
        if ((hInet->hRequest->wOptions & INET_FLAG_ICY_META_REQ) == INET_FLAG_ICY_META_REQ)
        {
            nResult = sprintf_P(&hInet->hRequest->pszRequest[hInet->hRequest->unRequestInUse],
                                PSTR("Icy-MetaData:1\r\n"));
            if (nResult >= 0)
            {
                hInet->hRequest->unRequestInUse += nResult;
                if (hInet->hRequest->unRequestInUse > hInet->hRequest->unRequestBufSize)
                {
                    nResult = -1;
                }
            }
        }
    }

    /*
     * Check if we need to close the connection
     */
    if (nResult >= 0)
    {
        if ((hInet->hRequest->wOptions & INET_FLAG_CLOSE) == INET_FLAG_CLOSE)
        {
            nResult = sprintf_P(&hInet->hRequest->pszRequest[hInet->hRequest->unRequestInUse],
                                PSTR("Connection: close\r\n"));
            if (nResult >= 0)
            {
                hInet->hRequest->unRequestInUse += nResult;
                if (hInet->hRequest->unRequestInUse > hInet->hRequest->unRequestBufSize)
                {
                    nResult = -1;
                }
            }
        }
    }

    /* Correct lenght in use for last \0 */
    if (hInet->hRequest->unRequestInUse > 0)
    {
        hInet->hRequest->unRequestInUse += 1;
    }

    /*
     * Log the request
     */
    if (nResult >= 0)
    {
//#ifdef INET_DEBUG
        LogMsg_P(LOG_DEBUG, PSTR("Request %u [%s]"), hInet->hRequest->unRequestInUse, hInet->hRequest->pszRequest);
//#endif /* #ifdef INET_DEBUG */
    }

    return (nResult);
}


/*!
 * \brief Get the HTTP response headers.
 *
 * This function returns after all response headers have been
 * received.
 *
 * \param   hInet [in] Handle returned by a previous call to InternetOpen.
 *
 * \return  -
 */
static void GetHeaders(HINET hInet)
{
    unsigned char byDone = 0;

    /*
     * Create room for buffers
     */
    char *pszRespLine = MyMalloc(HTTP_HEADER_LINE_SIZE);
    if (pszRespLine == NULL)
    {
        /* No memory */
        byDone = 1;
    }

    ShowDebug();

    if (hInet->hRequest == NULL)
    {
        /* Bad argument */
        byDone = 1;
    }
    else
    {
        /* Reset received counter */
        hInet->hRequest->unResponseInUse = 0;
    }

    /*
     * Process all header lines
     */
    while ((byDone == 0) &&
           (fgets(pszRespLine, HTTP_HEADER_LINE_SIZE, hInet->ptStream) != NULL))
    {
        /*
         * We could have been asleep; Check if we have received a close request
         */
        if (hInet->tState == INET_STATE_CLOSING)
        {
            byDone = 1;
        }
        else if ((pszRespLine[0] == '\r') && (pszRespLine[1] == '\n'))
        {
            /*
             * An empty line indicates the end of the headers
             */
            byDone = 1;
        }
        else
        {
            /*
             * Log the line (without the end of line stuff)
             */
            unsigned int unLength = 0;
            char *pszLogEol = strchr(pszRespLine, '\r');
            if (pszLogEol != NULL)
            {
                unLength = pszLogEol - pszRespLine;
            }
            LogMsg_P(LOG_DEBUG, PSTR("Read [%.*s]"), unLength, pszRespLine);

            if (BufferAddString(&hInet->hRequest->pszResponse,
                                &hInet->hRequest->unResponseBufSize,
                                &hInet->hRequest->unResponseInUse,
                                pszRespLine) != 0)
            {
                byDone = 1;
            }
        }
    }

    ShowDebug();
#ifdef INET_DEBUG
    LogMsg_P(LOG_DEBUG, PSTR("%d Read"), hInet->hRequest->unResponseInUse);
#endif /* #ifdef INET_DEBUG */

    /*
     * Cleanup
     */
    MyFree(pszRespLine);
}

/*!
 * \brief Close the socket and file descriptors of an INET handle.
 *
 * \param   hInet [in] Handle returned by a previous call to InternetOpen.
 *
 * \return  -
 */
static void CloseDescriptors(HINET hInet)
{
    if (hInet != NULL)
    {
        /*
         * Close the stream and the connection
         */
        if (hInet->ptStream != NULL)
        {
            (void)fclose(hInet->ptStream);
            hInet->ptStream = NULL;
        }
        if (hInet->ptSocket != NULL)
        {
            (void)NutTcpCloseSocket(hInet->ptSocket);
            hInet->ptSocket = NULL;
        }
    }
}




/*--------------------------------------------------------------------------*/
/*  Global functions                                                        */
/*--------------------------------------------------------------------------*/


HINET InetOpen(void)
{
    HINET hInet = NULL;

    ShowDebug();

    hInet = (HINET)MyMalloc(sizeof(INET));
    if (hInet != NULL)
    {
//        LogMsg_P(LOG_DEBUG, PSTR("Open %X"), hInet);

        memset(hInet, 0, sizeof(INET));
        hInet->tState = INET_STATE_IDLE;
    }

    return (hInet);
}

TError InetConnect(HINET hInet, CONST char *pszUrl, unsigned long ulRecvTimeout, unsigned int unMss, unsigned int unTcpRecvBufSize)
{
    TError tError = OK;

    ShowDebug();

    /*
     * Parse the Url
     */
    if (tError == OK)
    {
        hInet->pszUrl = strdup(pszUrl);
        if (hInet->pszUrl != NULL)
        {
            HttpParseUrl(hInet->pszUrl, &hInet->tUrlParts);
        }
        else
        {
            tError = INET_NOMEM;
        }
    }

    if (tError == OK)
    {
        /* Store the connect parameters */
        hInet->ulRecvTimeout = ulRecvTimeout;
        hInet->unMss = unMss;
        hInet->unTcpRecvBufSize = unTcpRecvBufSize;

        /* Reset the problem counters */
        memset(&hInet->tRetries, 0, sizeof(hInet->tRetries));

        tError = Connect(hInet);
    }

    return (tError);
}

TError InetHttpOpenRequest(HINET hInet, CONST char *pszMethod, CONST char *pszPath, CONST char *pszAccept, unsigned short wOptions)
{
    const unsigned int cunReqBufSize = 256;
    TError tError = OK;

    ShowDebug();

    if (hInet == NULL)
    {
        /* Bad argument */
        tError = INET_NOMEM;
    }

    /*
     * Create the request
     */
    if (tError == OK)
    {
        if (hInet->hRequest == NULL)
        {
            hInet->hRequest = (HINETREQ)MyMalloc(sizeof(INETREQ));
            if (hInet->hRequest == NULL)
            {
                /* No memory */
                tError = INET_NOMEM;
            }
            else
            {
                memset(hInet->hRequest, 0, sizeof(INETREQ));
            }
        }
    }

    /*
     * Create the request buffer
     */
    if (tError == OK)
    {
        if (hInet->hRequest->pszRequest == NULL)
        {
            /* Allocate the request buffer */
            hInet->hRequest->pszRequest = MyMalloc(cunReqBufSize);
            if (hInet->hRequest->pszRequest == NULL)
            {
                /* No memory */
                tError = INET_NOMEM;
            }
            else
            {
                hInet->hRequest->unRequestBufSize = cunReqBufSize;
            }
        }
    }

#ifdef INET_DEBUG
    LogMsg_P(LOG_DEBUG, PSTR("hInet @%X"), hInet);
    LogMsg_P(LOG_DEBUG, PSTR("hRequest @%X"), hInet->hRequest);
#endif /* #ifdef INET_DEBUG */

    /*
     * Create the actual request
     */
    if (tError == OK)
    {
        int nResult = 0;

        /* Store the requested options */
        hInet->hRequest->wOptions = wOptions;

        /* Create the request */
        nResult = CreateRequest(hInet, pszMethod, pszPath, pszAccept);
        if (nResult < 0)
        {
            tError = INET_NOMEM;
        }
    }

    ShowDebug();

    return (tError);
}

int InetHttpAddRequestHeaders(HINET hInet, CONST char *pszNewHeaders)
{
    int nResult = 0;

    ShowDebug();

    if ((hInet == NULL) || (hInet->hRequest == NULL))
    {
        /* Bad argument */
        nResult = -1;
    }

    if (nResult >= 0)
    {
        nResult = BufferAddString(&hInet->hRequest->pszRequest,
                                  &hInet->hRequest->unRequestBufSize,
                                  &hInet->hRequest->unRequestInUse,
                                  pszNewHeaders);
    }

#ifdef INET_DEBUG
    LogMsg_P(LOG_DEBUG, PSTR("Request %u [%s]"), hInet->hRequest->unRequestInUse, hInet->hRequest->pszRequest);
#endif /* #ifdef INET_DEBUG */
    ShowDebug();

    return (nResult);
}

TError InetHttpSendRequest(HINET hInet)
{
    TError tError = OK;
    unsigned char byDone = 0;
    unsigned char byRedirectCount = 0;
    int nHeaderNumber;
    long lResponseCode;
    void *plResponseCode;
    unsigned int unInfoSize;

    /*
     * Talk to the server and parse its reponse
     */
    while (byDone == 0)
    {
        int nResult = +1;
        int nResponse = -1;

        ShowDebug();

        if (hInet->tState == INET_STATE_CLOSING)
        {
            tError = USER_ABORT;
        }

        if (tError == OK)
        {
            hInet->tState = INET_STATE_BUSY;
        }

        /*
         * Send the request
         */
        if (tError == OK)
        {
            nResult = HttpSendRequest(hInet->ptStream, hInet->hRequest->pszRequest, hInet->hRequest->wHttpMode);
            if (nResult < 0)
            {
                tError = INET_SEND_FAIL;
            }
            LogMsg_P(LOG_DEBUG, PSTR("Sent %d"), nResult);
        }

        /*
         * We could have been asleep; Check if we still need to play
         */
        if (hInet->tState == INET_STATE_CLOSING)
        {
            tError = USER_ABORT;
        }

        /*
         * Get the response
         */
        if (tError == OK)
        {
            ShowDebug();

            GetHeaders(hInet);

            nHeaderNumber = 0;
            lResponseCode = 0;
            plResponseCode = &lResponseCode;
            unInfoSize = sizeof(lResponseCode);

            nResult = InetHttpQueryInfo(hInet,
                                        INET_HTTP_QUERY_STATUS_CODE | INET_HTTP_QUERY_MOD_NUMERIC,
                                        &plResponseCode,
                                        &unInfoSize,
                                        &nHeaderNumber);
            if (nResult > 0)
            {
                nResponse = lResponseCode;
            }
            else
            {
                /* Unable to process the response */
                nResponse = -1;
            }
            ShowDebug();

            /*
             * We could have been asleep; Check if we still need to play
             */
            if (hInet->tState == INET_STATE_CLOSING)
            {
                tError = USER_ABORT;
            }
        }

        if (tError == OK)
        {
            /*
             * Check the response code
             */
            if (nResponse >= 100 && nResponse < 300)
            {
                /* Connected: we're done */
                LogMsg_P(LOG_INFO, PSTR("Connect [%d]"), nResponse);
                byDone = 1;
            }
            else if (nResponse >= 300 && nResponse < 400)
            {
                /* Redirect */
                LogMsg_P(LOG_INFO, PSTR("Redirect [%d]"), nResponse);

                tError = INET_REDIRECT;
                /*
                 * Check if we can retry
                 */
                if (++byRedirectCount >= MAX_REDIRECT)
                {
                    /* Too many redirects, stop */
                    tError = INET_TOO_MANY_REDIRECTS;
                }
                else
                {
                    int nHeaderNumber = 0;
                    char *pszLocation = NULL;
                    void *ppszLocation = &pszLocation;
                    unsigned int unSize = 0;

                    /* Get size of the new location string */
                    nResult = InetHttpQueryInfo(hInet,
                                                INET_HTTP_QUERY_LOCATION,
                                                ppszLocation,
                                                &unSize,
                                                &nHeaderNumber);
                    /*
                     * (Re)parse the newly received URL
                     */
                    if (nResult > 0)
                    {
                        MyFree(hInet->pszUrl);
                        hInet->pszUrl = pszLocation;
                        LogMsg_P(LOG_INFO, PSTR("To [%s]"), hInet->pszUrl);
                        HttpParseUrl(hInet->pszUrl, &hInet->tUrlParts);

                        nResult = CreateRequest(hInet, NULL, NULL, NULL);
                    }

                    if (nResult >= 0)
                    {
                        /*
                         * Allow each server a maximum of problems :-)
                         */
                        memset(&hInet->tRetries, 0, sizeof(hInet->tRetries));
                    }
                    else
                    {
                        tError = INET_NOMEM;
                    }
                }
            }
            else if (nResponse == 401)
            {
                /*
                 * If we did not try authentication, try again with authentication.
                 * If we already tried using authentication or shouldn't use it, give up
                 */
                if (((hInet->hRequest->wHttpMode & HTTP_AUTH) != HTTP_AUTH) &&
                    ((hInet->hRequest->wOptions & INET_FLAG_NO_AUTH) != INET_FLAG_NO_AUTH))
                {
                    hInet->hRequest->wHttpMode |= HTTP_AUTH;
                    tError = INET_ACCESS_RESTRICTED;
                }
                else
                {
                    tError = INET_ACCESS_DENIED;
                    LogMsg_P(LOG_CRIT, PSTR("Access denied"));
                }
            }
            else
            {
                /* Bad response or timeout */
                LogMsg_P(LOG_ERR, PSTR("Bad response [%d]"), nResponse);

                tError = INET_BADRESPONSE;

                if (++hInet->tRetries.byBadResponseCount >= MAX_BADRESPONSE)
                {
                    /* Too many badresponses, stop */
                    tError = INET_TOO_MANY_BADRESPONSES;
                }
            }
        }

        if (tError != OK)
        {
            LogMsg_P(LOG_ERR, PSTR("Error [%d]"), tError);

            /*
             * Check if we need to try again
             */
            if ((tError > PLAYER_WARNINGS) && (tError < PLAYER_ERRORS))
            {
                LogMsg_P(LOG_INFO, PSTR("Retry"));

                CloseDescriptors(hInet);

                /*
                 * Try again
                 */
                tError = Connect(hInet);

                /*
                 * Give other threads some time before
                 * we try again
                 */
                NutSleep(300);
            }
            else
            {
                /* Errors, stop */
                byDone = 1;
            }
        }
    } /* end while */

    ShowDebug();

    hInet->tState = INET_STATE_IDLE;
    return (tError);
}


int InetHttpQueryInfo(HINET hInet, unsigned short wInfoLevel, void **pInfo, unsigned int *punInfoSize, int *pnIndex)
{
    /* Status prefixes: */
    static prog_char cszHttpVer_P[]         = "HTTP/";
    static prog_char cszIcy_P[]             = "ICY";
    /* HTTP headers: */
    static prog_char cszLocation_P[]        = "Location:";
    static prog_char cszContentLength_P[]   = "Content-Length:";
    static prog_char cszContentType_P[]     = "Content-Type:";
    /* ICY headers: */
    static prog_char cszIcyMetaData_P[]     = "icy-metaint:";

    int nResult = 0;
    char *pszRespLine = hInet->hRequest->pszResponse;
    char *pszStart = pszRespLine;
    char *pszEnd = pszRespLine;
    unsigned int unResultSize = 0;

    if ((pInfo == NULL) || (punInfoSize == NULL) || (pszRespLine == NULL))
    {
        /* Bad argument */
        nResult = -1;
    }

    while ((nResult == 0) && (pszEnd != NULL))
    {
        /* Skip empty lines and whitepace */
        while (isspace(*pszRespLine))
        {
            pszRespLine++;
        }

        pszEnd = strchr(pszRespLine, '\r');
        if (pszEnd != NULL)
        {
            if ((wInfoLevel & INET_HTTP_QUERY_STATUS_CODE) == INET_HTTP_QUERY_STATUS_CODE)
            {
                int iProcessed = 0;

                /*
                 * Check the response code in the first line
                 */
                if (strncasecmp_P(pszRespLine, cszHttpVer_P, sizeof(cszHttpVer_P)-1) == 0)
                {
                    hInet->hRequest->byProto = INET_PROTO_HTTP;
                    iProcessed = sizeof(cszHttpVer_P)-1;
                }
                else if (strncasecmp_P(pszRespLine, cszIcy_P, sizeof(cszIcy_P)-1) == 0)
                {
                    hInet->hRequest->byProto = INET_PROTO_ICY;
                    iProcessed = sizeof(cszIcy_P)-1;
                }

                if (iProcessed)
                {
                    nResult = 1;

                    /* Skip version number */
                    for (pszStart = pszRespLine + iProcessed; *pszStart != '\r' && *pszStart != ' '; pszStart++)
                    {
                        ;
                    }
                    if (*pszStart == '\r')
                    {
                        /* Could not find whitespace after the version number */
                        nResult = -1;
                    }
                    else
                    {
                        /* Skip leading whitespace */
                        for (; *pszStart == ' '; pszStart++)
                        {
                            ;
                        }
                        /* Strip trailing whitespace */
                        for (; ((pszEnd > pszStart) && (*(pszEnd-1) == ' ')); pszEnd--)
                        {
                            ;
                        }
                    }
                }
            }
            else if ((wInfoLevel & INET_HTTP_QUERY_LOCATION) == INET_HTTP_QUERY_LOCATION)
            {
                /*
                 * Get the new location (redirect URL)
                 */
                if (strncasecmp_P(pszRespLine, cszLocation_P, sizeof(cszLocation_P)-1) == 0)
                {
                    nResult = 1;

                    /* Skip leading whitespace */
                    for (pszStart = pszRespLine + sizeof(cszLocation_P)-1; (*pszStart == ' '); pszStart++)
                    {
                        ;
                    }

                    /* Strip trailing whitespace */
                    for (; ((pszEnd > pszStart) && (*(pszEnd-1) == ' ')); pszEnd--)
                    {
                        ;
                    }
                }
            }
            else if ((wInfoLevel & INET_HTTP_QUERY_CONTENT_LENGTH) == INET_HTTP_QUERY_CONTENT_LENGTH)
            {
                /*
                 * Get the content-length
                 */
                if (strncasecmp_P(pszRespLine, cszContentLength_P, sizeof(cszContentLength_P)-1) == 0)
                {
                    nResult = 1;

                    /* Skip leading whitespace */
                    for (pszStart = pszRespLine + sizeof(cszContentLength_P)-1; (*pszStart == ' '); pszStart++)
                    {
                        ;
                    }
                }
            }
            else if ((wInfoLevel & INET_HTTP_QUERY_CONTENT_TYPE) == INET_HTTP_QUERY_CONTENT_TYPE)
            {
                /*
                 * Get the content-type
                 */
                if (strncasecmp_P(pszRespLine, cszContentType_P, sizeof(cszContentType_P)-1) == 0)
                {
                    nResult = 1;

                    /* Skip leading whitespace */
                    for (pszStart = pszRespLine + sizeof(cszContentType_P)-1; (*pszStart == ' '); pszStart++)
                    {
                        ;
                    }
                }
            }
            else if ((wInfoLevel & INET_HTTP_QUERY_ICY_METADATA) == INET_HTTP_QUERY_ICY_METADATA)
            {
                /*
                 * Get the content-type
                 */
                if (strncasecmp_P(pszRespLine, cszIcyMetaData_P, sizeof(cszIcyMetaData_P)-1) == 0)
                {
                    nResult = 1;

                    /* Skip leading whitespace */
                    for (pszStart = pszRespLine + sizeof(cszIcyMetaData_P)-1; (*pszStart == ' '); pszStart++)
                    {
                        ;
                    }
                }
            }
        }
        /* Find next line */
        pszRespLine = pszEnd;
    } /* end while */

    /*
     * If we found what we are looking for, pass and optionally convert the resulting value to the caller
     */
    if (nResult > 0)
    {
        long lNumericValue = -1;

        if ((wInfoLevel & INET_HTTP_QUERY_MOD_NUMERIC) == INET_HTTP_QUERY_MOD_NUMERIC)
        {
            unResultSize = sizeof(lNumericValue);
        }
        else
        {
            unResultSize = pszEnd - pszStart;
            if (unResultSize > 0)
            {
                /* Correction so we can store the \0 */
                unResultSize += 1;
            }
        }

        /* Allocate a buffer if the caller did not */
        if (*punInfoSize == 0)
        {
            *pInfo = MyMalloc(unResultSize);
            *punInfoSize = unResultSize;
        }

        /* Copy if there is room */
        if ((*punInfoSize >= unResultSize) &&
            (*pInfo != NULL))
        {
            if ((wInfoLevel & INET_HTTP_QUERY_MOD_NUMERIC) == INET_HTTP_QUERY_MOD_NUMERIC)
            {
                long *plDest = *pInfo;
                lNumericValue = strtol(pszStart, (char **) NULL, 0);
                *plDest = lNumericValue;
            }
            else
            {
                if (unResultSize > 0)
                {
                    char *pszDest = *pInfo;
                    memcpy(pszDest, pszStart, unResultSize-1);
                    pszDest[unResultSize-1] = '\0';
                }
            }
        }
        else
        {
            /* No room to store the result */
            nResult = -1;
        }
        *punInfoSize = unResultSize;
    }
    return (nResult);
}

/*\brief Mime types */
static prog_char cszTypeAudio_P[]   = "audio/";
static prog_char cszTypeText_P[]    = "text/";

/*\brief Audio subtypes */
static prog_char cszTypeM3u_P[]     = "x-mpegurl";
static prog_char cszTypePls_P[]     = "x-scpls";
//static prog_char cszSubtypeWma[]  = "x-ms-wma";
//static prog_char cszSubtypeMp3[]  = "mpeg";
//static prog_char cszSubtypeAac[]  = "aacp";

/*\brief File extensions */
static prog_char cszPlsExtension_P[]    = "pls";
static prog_char cszM3uExtension_P[]    = "m3u";

int InetGetMimeType(HINET hInet)
{
    int nResult = +1;
    int nType = MIME_TYPE_UNKNOWN;
    char *pszContentType = NULL;

    /*
     * Try to determine the filetype based on the content type header
     */
    int nHeaderNumber = 0;
    void *ppszContentType = &pszContentType;
    unsigned int unInfoSize = 0;

    nResult = InetHttpQueryInfo(hInet,
                                INET_HTTP_QUERY_CONTENT_TYPE,
                                ppszContentType,
                                &unInfoSize,
                                &nHeaderNumber);
    if (nResult > 0)
    {
        if (strncasecmp_P(pszContentType, cszTypeAudio_P, sizeof(cszTypeAudio_P)-1) == 0)
        {
            nType = MIME_TYPE_MP3;

            char *pszSubType = pszContentType + sizeof(cszTypeAudio_P)-1;
            if (strncasecmp_P(pszSubType, cszTypePls_P, sizeof(cszTypePls_P)-1) == 0)
            {
                 nType = MIME_TYPE_PLS;
            }
            else if (strncasecmp_P(pszSubType, cszTypeM3u_P, sizeof(cszTypeM3u_P)-1) == 0)
            {
                nType = MIME_TYPE_M3U;
            }
        }
        else if (strncasecmp_P(pszContentType, cszTypeText_P, sizeof(cszTypeText_P)-1) == 0)
        {
            /* Assume the generic text type */
            nType = MIME_TYPE_TEXT;
        }
    }
    else
    {
        nType = MIME_TYPE_MP3;
    }

    /*
     * Icecast servers serve only audio
     */
    if ((nType == MIME_TYPE_UNKNOWN) && (hInet->hRequest->byProto == INET_PROTO_ICY))
    {
        nType = MIME_TYPE_MP3;
    }

    /*
     * If all else fails, use the extension of the path
     */
    if ((nType == MIME_TYPE_UNKNOWN) || (nType == MIME_TYPE_TEXT))
    {
        /* Assume the extension starts at the last '.' we can find */
        char *szExtension = strrchr(hInet->tUrlParts.pszPath, '.');
        if (szExtension != NULL)
        {
            szExtension++; /* Skip the . */
            if (strncasecmp_P(szExtension, cszPlsExtension_P, sizeof(cszPlsExtension_P)-1) == 0)
            {
                nType = MIME_TYPE_PLS;
            }
            else if (strncasecmp_P(szExtension, cszM3uExtension_P, sizeof(cszM3uExtension_P)-1) == 0)
            {
                nType = MIME_TYPE_M3U;
            }
        }
    }

    LogMsg_P(LOG_INFO, PSTR("File type %d"), nType);

    MyFree(pszContentType);

    return (nType);
}

int InetRead(HINET hInet, char *pcBuf, unsigned int unBufSize)
{
    int nResult = -1;

    if (hInet != NULL)
    {
        if (hInet->tState != INET_STATE_CLOSING)
        {
            hInet->tState = INET_STATE_BUSY;

            //nResult = _read((int)hInet->ptSocket, pcBuf, unBufSize);
            nResult = NutTcpReceive(hInet->ptSocket, pcBuf, unBufSize);
            if (nResult < 0)
            {
                /*
                 * Either an error occurred or the other side closed the connection (= EOF).
                 */
                LogMsg_P(LOG_INFO, PSTR("EOF %d"), NutTcpError(hInet->ptSocket));
            }
            else if (nResult == 0)
            {
                LogMsg_P(LOG_INFO, PSTR("Read Timeout"));
            }
        }

        hInet->tState = INET_STATE_IDLE;
    }
    return (nResult);
}

int InetReadExact(HINET hInet, unsigned char *pbyBuf, unsigned int unBufSize)
{
    int nResult = +1;
    unsigned int unBufInUse = 0;

    /* Sanity check */
    if ((hInet == NULL) || (pbyBuf == NULL))
    {
        return (-1);
    }

    /* Keep reading until we are done */
    while ((nResult > 0) && (unBufSize-unBufInUse) > 0)
    {
        nResult = InetRead(hInet, (char*)&pbyBuf[unBufInUse], unBufSize-unBufInUse);
        //LogMsg_P(LOG_DEBUG, PSTR("r %d,max %u"), nResult, unBufSize-unBufInUse);

        if (nResult > 0)
        {
            unBufInUse += nResult;
        }
    }
    return (unBufInUse);
}

int InetReadFile(HINET hInet, char **ppcBuf, unsigned int *punBufSize)
{
    int nResult = +1;
    unsigned int unBufInUse = 0;
    unsigned char byDoResize = 0;

    /* Sanity check */
    if ((ppcBuf == NULL) || (punBufSize == NULL))
    {
        nResult = -1;
    }

    /* If you did not provide a buffer, one will be provided for you */
    if (nResult > 0)
    {
        if (*ppcBuf == NULL)
        {
            byDoResize = 1;
            *punBufSize = 0;
        }
    }

    /* Keep reading until we are done */
    while (nResult > 0)
    {
        if (byDoResize)
        {
            nResult = BufferMakeRoom((char**)ppcBuf, punBufSize, unBufInUse, 100);
            //LogMsg_P(LOG_DEBUG, PSTR("r %d,size %u, %x"), nResult, *punBufSize, &(*ppcBuf)[unBufInUse]);
        }

        if (nResult >= 0)
        {
            nResult = InetRead(hInet, (char*)&(*ppcBuf)[unBufInUse], *punBufSize-unBufInUse);
            //LogMsg_P(LOG_DEBUG, PSTR("r %d,max %u"), nResult, *punBufSize-unBufInUse);
        }

        if (nResult > 0)
        {
            unBufInUse += nResult;
        }
        else
        {
            /*
             * We have a eof/disconnect (-1), or a read timeout (0)
             */
        }
    }

    if (unBufInUse == 0)
    {
        LogMsg_P(LOG_WARNING, PSTR("No data"));
    }
    return (unBufInUse);
}

HINET InetClose(HINET hInet)
{
//    LogMsg_P(LOG_DEBUG, PSTR("Close %X %d"), hInet, hInet->tState);

    if (hInet != NULL)
    {
        if (hInet->tState != INET_STATE_IDLE)
        {
            hInet->tState = INET_STATE_CLOSING;

            /*
             * Wait for the close to be handled
             */
            while (hInet->tState != INET_STATE_IDLE)
            {
                unsigned int nCount = 0;

                NutSleep(100);

                /* After 10 seconds */
                if (++nCount == 100)
                {
                    LogMsg_P(LOG_EMERG, PSTR("Close failed"));
                    break;
                }
            }
        }

        CloseDescriptors(hInet);
        MyFree(hInet->pszUrl);
        ShowDebug();

        /*
         * Deallocate the request struct
         */
        if (hInet->hRequest != NULL)
        {
            MyFree(hInet->hRequest->pszRequest);
            ShowDebug();
            MyFree(hInet->hRequest->pszResponse);
            ShowDebug();
            MyFree(hInet->hRequest);
            ShowDebug();
        }
        MyFree(hInet);
    }
    ShowDebug();
    return (hInet);
}

