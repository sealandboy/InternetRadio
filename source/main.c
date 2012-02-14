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
 * $Log: httpserv.c,v $
 * Revision 1.16  2007/07/17 18:29:30  haraldkipp
 * Server thread names not unique on SAM7X. Fixed by Marti Raudsepp.
 *
 * Revision 1.15  2006/09/07 09:01:36  haraldkipp
 * Discovery registration added.
 * Re-arranged network interface setup to exclude DHCP code from ICCAVR
 * builds and make it work with the demo compiler. Unfinished.
 * Added PHAT file system support. Untested.
 *
 * Revision 1.14  2006/03/02 19:44:03  haraldkipp
 * MMC and PHAT enabled.
 *
 * Revision 1.13  2006/01/11 08:32:57  hwmaier
 * Added explicit type casts to silence a few avr-gcc 3.4.3 warning messages
 *
 * Revision 1.12  2005/11/22 09:14:13  haraldkipp
 * Replaced specific device names by generalized macros.
 *
 * Revision 1.11  2005/10/16 23:22:20  hwmaier
 * Removed unreferenced nutconfig.h include statement
 *
 * Revision 1.10  2005/08/05 11:32:50  olereinhardt
 * Added SSI and ASP sample
 *
 * Revision 1.9  2005/04/05 18:04:17  haraldkipp
 * Support for ARM7 Wolf Board added.
 *
 * Revision 1.8  2005/02/23 04:39:26  hwmaier
 * no message
 *
 * Revision 1.7  2005/02/22 02:44:34  hwmaier
 * Changes to compile as well for AT90CAN128 device.
 *
 * Revision 1.6  2004/12/16 10:17:18  haraldkipp
 * Added Mikael Adolfsson's excellent parameter parsing routines.
 *
 * Revision 1.5  2004/03/16 16:48:26  haraldkipp
 * Added Jan Dubiec's H8/300 port.
 *
 * Revision 1.4  2003/11/04 17:46:52  haraldkipp
 * Adapted to Ethernut 2
 *
 * Revision 1.3  2003/09/29 16:33:12  haraldkipp
 * Using portable strtok and strtok_r
 *
 * Revision 1.2  2003/08/07 08:27:58  haraldkipp
 * Bugfix, remote not displayed in socket list
 *
 * Revision 1.1  2003/07/20 15:56:14  haraldkipp
 * *** empty log message ***
 *
 */

/*!
 * \example httpd/httpserv.c
 *
 * Simple multithreaded HTTP daemon.
 */


#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include <dev/x12rtc.h>
#include <dev/nicrtl.h>
#include "main.h"
#include <dev/urom.h>
#include <dev/nplmmc.h>
#include <dev/sbimmc.h>
#include <fs/phatfs.h>

#include <sys/version.h>
#include <sys/thread.h>
#include <sys/timer.h>
#include <sys/heap.h>
#include <sys/confnet.h>
#include <sys/socket.h>


#include <arpa/inet.h>
#include <net/route.h>
#include <netinet/tcp.h>

#include <pro/httpd.h>
#include <pro/dhcp.h>
#include <pro/ssi.h>
#include <pro/asp.h>
#include <pro/sntp.h>
#include <pro/discover.h>

#define ETH0_BASE	0xC300
#define ETH0_IRQ	5

#define OK			1
#define NOK			0


#ifdef NUTDEBUG
#include <sys/osdebug.h>
#include <net/netdebug.h>
#endif

/* Service thread counter. */
static int httpd_tc;

static char *html_mt = "text/html";

/*
 * Write HTML page header to a specified stream.
 */
static void WriteHtmlPageHeader(FILE *stream, char *title)
{
    /*
     * This may look a little bit weird if you are not familiar with C 
     * programming for flash microcontrollers based on the Harvard 
     * architecture. The special type 'prog_char' forces the string 
     * literals to be placed in Flash memory. This saves us a lot of
     * precious RAM.
     *
     * On non Harvard architectures this is replaced by the normal 
     * character type.
     */
    static prog_char title_fmt_P[] = "<html><head><title>%s</title></head>" /* */
        "<body><h1><a href=\"/index.html\"><img border=0 src=\"/nutlogo.png\" width=48 height=56 align=left>" /* */
        "</a>nut/os<br>%s</h1>\r\n";

    /* 
     * Note, that we need to call special variants of standard I/O
     * routines when using Flash memory pointers. Here we use
     * fprintf_P() instead of fprintf().
     */
    fprintf_P(stream, title_fmt_P, title, title);

    /*
     * Stream output may be buffered. Make sure to send the title
     * before returning. If nothing else than the title appears
     * on the browser, than we can assume that the CGI code has
     * a problem.
     */
    fflush(stream);
}

/*
 * Write HTTP response and HTML page header to a specified stream.
 */
 
 NUTRTC rtcX12x6 = {
     X12Init,            
     X12RtcGetClock,     
     X12RtcSetClock,     
     X12RtcGetAlarm,     
     X12RtcSetAlarm,     
     X12RtcGetStatus,    
     X12RtcClearStatus   
 };
static void WriteHtmlIntro(FILE *stream, REQUEST * req, char *title)
{
    /* These useful API calls create a HTTP response for us. */
    NutHttpSendHeaderTop(stream, req, 200, "Ok");
    NutHttpSendHeaderBottom(stream, req, html_mt, -1);

    /* Send HTML header. */
    WriteHtmlPageHeader(stream, title);
}

#ifdef USE_DATE_AND_TIME
/*
 * Print current local date to the specified stream.
 */
static void WriteLocalDate(FILE *stream)
{
    time_t now = time(NULL);
    struct _tm *lot = localtime(&now);
    fprintf(stream, "%02u.%02u.%u", lot->tm_mday, lot->tm_mon + 1, 1900 + lot->tm_year);
}

/*
 * Print current local daytime to the specified stream.
 */
static void WriteLocalTime(FILE *stream)
{
    time_t now = time(NULL);
    struct _tm *lot = localtime(&now);
    fprintf(stream, "%02u:%02u:%02u\n", lot->tm_hour, lot->tm_min, lot->tm_sec);
}
#endif


#ifdef USE_ASP
/*
 * ASPCallback
 *
 * This routine must have been registered by NutRegisterAspCallback() 
 * and is automatically called by NutHttpProcessRequest() when the 
 * server processes a page with an ASP function.
 *
 * Return 0 on success, -1 otherwise.
 */
static int ASPCallback (char *pASPFunction, FILE *stream)
{
    if (strcmp(pASPFunction, "usr_date") == 0)
        WriteLocalDate(stream);
    else if (strcmp(pASPFunction, "usr_time") == 0)
        WriteLocalTime(stream);
    else
        return -1;
    return 0;
}
#endif

/*
 * CGI Sample: Display request request information structure.
 *
 * See httpd.h for REQUEST structure.
 *
 * This routine must have been registered by NutRegisterCgi() and is
 * automatically called by NutHttpProcessRequest() when the client
 * request the URL 'cgi-bin/test.cgi'.
 */
static int ShowQuery(FILE * stream, REQUEST * req)
{
    char *cp;
    static prog_char foot_P[] = "</BODY></HTML>";
    static prog_char req_fmt_P[] = "Method: %s<BR>\r\nVersion: HTTP/%d.%d<BR>\r\nLength: %ld<BR>\r\n";
    static prog_char url_fmt_P[] = "URL: %s<BR>\r\n";
    static prog_char query_fmt_P[] = "Argument: %s<BR>\r\n";
    static prog_char type_fmt_P[] = "Content: %s<BR>\r\n";
    static prog_char cookie_fmt_P[] = "Cookie: %s<BR>\r\n";
    static prog_char auth_fmt_P[] = "Auth: %s<BR>\r\n";
    static prog_char agent_fmt_P[] = "Agent: %s<BR>\r\n";
    static prog_char referer_fmt_P[] = "Referrer: %s<BR>\r\n";
    static prog_char host_fmt_P[] = "Host: %s<BR>\r\n";
    static prog_char conn_fmt_P[] = "Connection: %s<BR>\r\n";

    /* Send headers and titles. */
    WriteHtmlIntro(stream, req, "Request Info");

    /*
     * Send request parameters.
     */
    switch (req->req_method) {
    case METHOD_GET:
        cp = "GET";
        break;
    case METHOD_POST:
        cp = "POST";
        break;
    case METHOD_HEAD:
        cp = "HEAD";
        break;
    default:
        cp = "UNKNOWN";
        break;
    }
    fprintf_P(stream, req_fmt_P, cp, req->req_version / 10, req->req_version % 10, req->req_length);
    if (req->req_url)
        fprintf_P(stream, url_fmt_P, req->req_url);
    if (req->req_query)
        fprintf_P(stream, query_fmt_P, req->req_query);
    if (req->req_type)
        fprintf_P(stream, type_fmt_P, req->req_type);
    if (req->req_cookie)
        fprintf_P(stream, cookie_fmt_P, req->req_cookie);
    if (req->req_auth)
        fprintf_P(stream, auth_fmt_P, req->req_auth);
    if (req->req_agent)
        fprintf_P(stream, agent_fmt_P, req->req_agent);
    if (req->req_referer)
        fprintf_P(stream, referer_fmt_P, req->req_referer);
    if (req->req_host)
        fprintf_P(stream, host_fmt_P, req->req_host);
#ifdef USE_DATE_AND_TIME
    if (req->req_ims) {
        static prog_char ifmod_fmt_P[] = "If-Modified-Since: %02u.%02u.%u %02u:%02u:%02u<BR>\r\n";
        struct _tm *lot = localtime(&req->req_ims);
        fprintf_P(stream, ifmod_fmt_P, lot->tm_mday, lot->tm_mon + 1, 1900 + lot->tm_year,
            lot->tm_hour, lot->tm_min, lot->tm_sec);
    }
#endif
    fprintf_P(stream, conn_fmt_P, req->req_connection == HTTP_CONN_KEEP_ALIVE ? "Keep-Alive" : "Close");

    /* Send HTML footer and flush output buffer. */
    fputs_P(foot_P, stream);
    fflush(stream);

    return 0;
}

/*
 * CGI Sample: Show list of threads.
 *
 * This routine must have been registered by NutRegisterCgi() and is
 * automatically called by NutHttpProcessRequest() when the client
 * request the URL 'cgi-bin/threads.cgi'.
 */
static int ShowThreads(FILE * stream, REQUEST * req)
{
    static prog_char mem_fmt_P[] = "%lu bytes free<p>";
    static prog_char tabhead_P[] = "<TABLE BORDER><TR><TH>Handle</TH><TH>Name</TH><TH>Priority</TH><TH>Status</TH><TH>Event<BR>Queue</TH><TH>Timer</TH><TH>Stack-<BR>pointer</TH><TH>Free<BR>Stack</TH></TR>\r\n";
    static prog_char tfmt[] =
        "<TR><TD>%p</TD><TD>%s</TD><TD>%u</TD><TD>%s</TD><TD>%p</TD><TD>%p</TD><TD>%p</TD><TD>%u</TD><TD>%s</TD></TR>\r\n";
    static prog_char foot[] = "</TABLE></BODY></HTML>";
    static char *thread_states[] = { "TRM", "<FONT COLOR=#CC0000>RUN</FONT>", "<FONT COLOR=#339966>RDY</FONT>", "SLP" };
    NUTTHREADINFO *tdp = nutThreadList;
    NUTTHREADINFO **list;
    int num;
    int i;

    /* Send headers and titles. */
    WriteHtmlIntro(stream, req, "Threads");

    /* Display memory status. */
    fprintf_P(stream, mem_fmt_P, (u_long)NutHeapAvailable());

    /*
     * We must not write to the socket stream while walking through the
     * linked list of threads. A write may block and we may lose the CPU.
     * New threads may be created, existing ones may disappear and the
     * link pointers may become invalid.
     *
     * To avoid getting trapped by invalid links, we collect all pointers 
     * to the thread info structures before starting the output. It may
     * still happen, that we display an invalid structure, but we will
     * no longer get trapped by bad link pointers.
     */
    for (num = 0, tdp = nutThreadList; num < 100 && tdp; num++, tdp = tdp->td_next);
    list = malloc(sizeof(NUTTHREADINFO *) * num);
    if (list) {
        for (i = 0, tdp = nutThreadList; i < num && tdp; i++, tdp = tdp->td_next) {
            list[i] = tdp;
        }

        /* Send our list in an HTML table. */
        fputs_P(tabhead_P, stream);
        for (i = 0; i < num; i++) {
            tdp = list[i];
            fprintf_P(stream, tfmt, tdp, tdp->td_name, tdp->td_priority,
                      thread_states[tdp->td_state], tdp->td_queue, tdp->td_timer,
                      (void *)tdp->td_sp, (u_int)((uptr_t)tdp->td_sp - (uptr_t)tdp->td_memory),
                    *((u_long *) tdp->td_memory) != DEADBEEF ? "Corr" : "OK");
        }
        free(list);
    }
    fputs_P(foot, stream);
    fflush(stream);

    return 0;
}

/*
 * CGI Sample: Show list of timers.
 *
 * This routine must have been registered by NutRegisterCgi() and is
 * automatically called by NutHttpProcessRequest() when the client
 * request the URL 'cgi-bin/timers.cgi'.
 */
static int ShowTimers(FILE * stream, REQUEST * req)
{
    static prog_char clock_fmt_P[] = "CPU Clock: %luHz<br>System tick: %lums<p>";
    static prog_char thead[] =
        "<TABLE BORDER><TR><TH>Handle</TH><TH>Countdown</TH><TH>Tick Reload</TH><TH>Callback<BR>Address</TH><TH>Callback<BR>Argument</TH></TR>\r\n";
    static prog_char tfmt[] = "<TR><TD>%p</TD><TD>%lu</TD><TD>%lu</TD><TD>%p</TD><TD>%p</TD></TR>\r\n";
    static prog_char foot[] = "</TABLE></BODY></HTML>";
    NUTTIMERINFO *tnp;
    NUTTIMERINFO **list;
    int num;
    int i;
    u_long ticks_left;

    /* Send headers and titles. */
    WriteHtmlIntro(stream, req, "Timers");

    /* Display clock status. */
    fprintf_P(stream, clock_fmt_P, (u_long)NutGetCpuClock(), (u_long)NutGetMillis());

    /* Create a local list first. See ShowThreads() for further informations. */
    for (num = 0, tnp = nutTimerList; num < 100 && tnp; num++, tnp = tnp->tn_next);
    if (num) {
        list = malloc(sizeof(NUTTIMERINFO *) * num);
        if (list) {
            for (i = 0, tnp = nutTimerList; i < num && tnp; i++, tnp = tnp->tn_next) {
                list[i] = tnp;
            }

            fputs_P(thead, stream);
            ticks_left = 0;
            for (i = 0; i < num; i++) {
                tnp = list[i];
                ticks_left += tnp->tn_ticks_left;
                fprintf_P(stream, tfmt, tnp, ticks_left, tnp->tn_ticks, tnp->tn_callback, tnp->tn_arg);
            }
        }
    }

    fputs_P(foot, stream);
    fflush(stream);

    return 0;
}

/*
 * CGI Sample: Show list of sockets.
 *
 * This routine must have been registered by NutRegisterCgi() and is
 * automatically called by NutHttpProcessRequest() when the client
 * request the URL 'cgi-bin/sockets.cgi'.
 */
static int ShowSockets(FILE * stream, REQUEST * req)
{
    /* String literals are kept in flash ROM. */
    static prog_char tabhead_P[] = "<TABLE BORDER><TR><TH>Handle</TH><TH>Type</TH><TH>Local</TH><TH>Remote</TH><TH>Status</TH></TR>\r\n";
    static prog_char sockinfo_fmt_P[] = "<TR><TD>%p</TD><TD>TCP</TD><TD>%s:%u</TD><TD>%s:%u</TD><TD>";
    static prog_char rowend_P[] = "</TD></TR>\r\n";
    static prog_char foot_P[] = "</TABLE></BODY></HTML>";
    static prog_char st_listen_P[] = "LISTEN";
    static prog_char st_synsent_P[] = "SYNSENT";
    static prog_char st_synrcvd_P[] = "SYNRCVD";
    static prog_char st_estab_P[] = "<FONT COLOR=#CC0000>ESTABL</FONT>";
    static prog_char st_finwait1_P[] = "FINWAIT1";
    static prog_char st_finwait2_P[] = "FINWAIT2";
    static prog_char st_closewait_P[] = "CLOSEWAIT";
    static prog_char st_closing_P[] = "CLOSING";
    static prog_char st_lastack_P[] = "LASTACK";
    static prog_char st_timewait_P[] = "TIMEWAIT";
    static prog_char st_closed_P[] = "CLOSED";
    static prog_char st_unknown_P[] = "UNKNOWN";
    prog_char *st_P;
    extern TCPSOCKET *tcpSocketList;
    TCPSOCKET **list;
    TCPSOCKET *ts;
    int num;
    int i;

    /* Send headers and titles. */
    WriteHtmlIntro(stream, req, "Sockets");

    /* Create a local list first. See ShowThreads() for further informations. */
    for (num = 0, ts = tcpSocketList; num < 100 && ts; num++, ts = ts->so_next);
    if (num) {
        list = malloc(sizeof(TCPSOCKET *) * num);
        if (list) {
            for (i = 0, ts = tcpSocketList; i < num && ts; i++, ts = ts->so_next) {
                list[i] = ts;
            }
            /* HTML table header. */
            fputs_P(tabhead_P, stream);
            for (i = 0; i < num; i++) {
                ts = list[i];

                /* Determine socket state. */
                switch (ts->so_state) {
                case TCPS_LISTEN:
                    st_P = st_listen_P;
                    break;
                case TCPS_SYN_SENT:
                    st_P = st_synsent_P;
                    break;
                case TCPS_SYN_RECEIVED:
                    st_P = st_synrcvd_P;
                    break;
                case TCPS_ESTABLISHED:
                    st_P = st_estab_P;
                    break;
                case TCPS_FIN_WAIT_1:
                    st_P = st_finwait1_P;
                    break;
                case TCPS_FIN_WAIT_2:
                    st_P = st_finwait2_P;
                    break;
                case TCPS_CLOSE_WAIT:
                    st_P = st_closewait_P;
                    break;
                case TCPS_CLOSING:
                    st_P = st_closing_P;
                    break;
                case TCPS_LAST_ACK:
                    st_P = st_lastack_P;
                    break;
                case TCPS_TIME_WAIT:
                    st_P = st_timewait_P;
                    break;
                case TCPS_CLOSED:
                    st_P = st_closed_P;
                    break;
                default:
                    st_P = st_unknown_P;
                    break;
                }
                /* Print infos about this socket. */
                fprintf_P(stream, sockinfo_fmt_P, ts, inet_ntoa(ts->so_local_addr), ntohs(ts->so_local_port),
                          inet_ntoa(ts->so_remote_addr), ntohs(ts->so_remote_port));
                fputs_P(st_P, stream);
                fputs_P(rowend_P, stream);
            }
        }
    }
    fputs_P(foot_P, stream);
    fflush(stream);

    return 0;
}

#ifdef USE_CGI_PARAMETERS
/*
 * CGI Sample: Proccessing a form.
 *
 * This routine must have been registered by NutRegisterCgi() and is
 * automatically called by NutHttpProcessRequest() when the client
 * request the URL 'cgi-bin/form.cgi'.
 *
 * Thanks to Tom Boettger, who provided this sample for ICCAVR.
 */
int ShowForm(FILE * stream, REQUEST * req)
{
    static prog_char html_body[] = "</BODY></HTML>";

    /* Send headers and titles. */
    WriteHtmlIntro(stream, req, "Form Result");

    if (req->req_query) {
        char *name;
        char *value;
        int i;
        int count;

        count = NutHttpGetParameterCount(req);
        /* Extract count parameters. */
        for (i = 0; i < count; i++) {
            name = NutHttpGetParameterName(req, i);
            value = NutHttpGetParameterValue(req, i);

            /* Send the parameters back to the client. */
            fprintf(stream, "%s: %s<br>\r\n", name, value);
        }
    }

    fputs_P(html_body, stream);
    fflush(stream);

    return 0;
}
#endif /* USE_CGI_PARAMETERS */

static void StartServiceThread(void);

/*
 * HTTP service thread.
 *
 * Nut/Net doesn't support a server backlog. If one client has established 
 * a connection, further connect attempts will be rejected. Thus, we need
 * to start more than one instance of this thread.
 */
THREAD(Service, arg)
{
    TCPSOCKET *sock;
    FILE *stream;
    int wcntr;
    u_int id = (u_int) ((uptr_t) arg);

    /*
     * Each loop serves a single connection.
     */
    for (;;) {

        /* Create a socket. */
        if ((sock = NutTcpCreateSocket()) == 0) {
            printf("[%u] Creating socket failed\n", id);
            NutSleep(5000);
            continue;
        }

        /* Set socket options. */
#ifdef HTTPD_MAX_SEGSIZE
        {
            u_short mss = HTTPD_MAX_SEGSIZE;
            if (NutTcpSetSockOpt(sock, TCP_MAXSEG, &mss, sizeof(mss)))
                printf("Sockopt MSS failed\n");
        }
#endif
#ifdef HTTPD_TCP_BUFSIZE
        {
            u_short tcpbufsiz = HTTPD_TCP_BUFSIZE;
            if (NutTcpSetSockOpt(sock, SO_RCVBUF, &tcpbufsiz, sizeof(tcpbufsiz)))
                printf("Sockopt rxbuf failed\n");
        }
#endif
#ifdef HTTPD_TCP_TIMEOUT
        {
            u_long tmo = HTTPD_TCP_TIMEOUT;
            if (NutTcpSetSockOpt(sock, SO_RCVTIMEO, &tmo, sizeof(tmo)))
                printf("Sockopt rx timeout failed\n");
        }
#endif


        /*
         * Listen on the configured port. NutTcpAccept() will block until we 
         * get a connection from a client.
         */
        if (NutTcpAccept(sock, HTTPD_TCP_PORT) == 0) {
#ifdef HTTPD_VERBOSE
            printf("[%u] Connected, %lu bytes free\n", id, (u_long)NutHeapAvailable());
#endif

            /*
             * Wait until at least 8 kByte of free RAM is available. This will
             * keep the client connected in low memory situations.
             */
            wcntr = 10;
            while (NutHeapAvailable() < 8192) {
                if (wcntr == 10)
                    printf("[%u] Mem low\n", id);
                if (wcntr--)
                    NutSleep(10);
                else 
                    break;
            }
            
            if (wcntr >= 0) {
                /*
                 * Associate a stream with the socket so we can use standard I/O calls.
                 */
                wcntr = 10;
                while ((stream = _fdopen((int) ((uptr_t) sock), "r+b")) == NULL) {
                    if (wcntr == 10)
                        printf("[%u] Streams low\n", id);
                    if (wcntr--)
                        NutSleep(10);
                    else
                        break;
                }

                if (stream) {
#ifdef USE_DYNAMIC_THREADS
                    /* Resources are fine, start a new thread and let it run. */
                    StartServiceThread();
                    NutSleep(1);
#endif
                    /*
                     * This API call saves us a lot of work. It will parse the
                     * client's HTTP request, send any requested file from the
                     * registered file system or handle CGI requests by calling
                     * our registered CGI routine.
                     */
                    NutHttpProcessRequest(stream);

                    /*
                     * Destroy the virtual stream device.
                     */
                    fclose(stream);
                } else {
                    printf("[%u] No stream\n", id);
                }
            }
            else
                printf("[%u] No mem\n", id);
        }

        /*
         * Close our socket.
         */
        NutTcpCloseSocket(sock);
#ifdef HTTPD_VERBOSE
        printf("[%u] Disconnected\n", id);
#endif

#ifdef USE_DYNAMIC_THREADS
        /* If enough threads are running, stop this one. */
        if (httpd_tc >= HTTPD_MIN_THREADS) {
            httpd_tc--;
            NutThreadExit();
        }
#endif
    }
}

/*
 * Start a HTTP daemon thread.
 */
static void StartServiceThread(void)
{
    static int httpd_id;

#ifdef USE_DYNAMIC_THREADS
    if (httpd_tc >= HTTPD_MAX_THREADS) {
        return;
    }
#endif

    if (NutThreadCreate("httpd", Service, (void *) (uptr_t) ++httpd_id, HTTPD_SERVICE_STACK)) {
        httpd_tc++;
    } else {
        printf("[%u] No thread\n", httpd_id);
    }
}

#ifdef USE_DATE_AND_TIME
/*
 * Try to get initial date and time from the hardware clock or a time server.
 */
static int InitTimeAndDate(void)
{
    int rc = -1;

    /* Set the local time zone. */
    _timezone = MYTZ * 60L * 60L;

#ifdef RTC_CHIP
    /* Register and query hardware RTC, if available. */
    printf("Init RTC...");
    if (NutRegisterRtc(&RTC_CHIP)) {
        puts("failed");
    } else {
        u_long rtc_stat;

        NutRtcGetStatus(&rtc_stat);
        if (rtc_stat & RTC_STATUS_PF) {
            puts("time lost");
        }
        else {
            puts("OK");
            rc = 0;
        }
    }
#endif /* RTC_CHIP */

#ifdef MYTIMED
    if (rc) {
        time_t now;
        u_long timeserver = inet_addr(MYTIMED);
        int trials = 5;

        while (trials--) {
            /* Query network time service and set the system time. */
            printf("Query time from %s...", MYTIMED);
            if(NutSNTPGetTime(&timeserver, &now) == 0) {
                puts("OK");
                rc = 0;
                stime(&now);
                break;
            }
            else {
                puts("failed");
            }
        }
    }
#endif /* MYTIMED */

    return rc;
}
#endif

/*!
 * \brief Main application routine.
 *
 * Nut/OS automatically calls this entry after initialization.
 */
int main(void)
{
    u_long baud = 115200;
    int i;

    /*
     * Initialize the uart device.
     */
    NutRegisterDevice(&DEV_DEBUG, 0, 0);
    freopen(DEV_DEBUG_NAME, "w", stdout);
    _ioctl(_fileno(stdout), UART_SETSPEED, &baud);
    NutSleep(200);
    printf("\n\nNut/OS %s HTTP Daemon...", NutVersionString());

#ifdef NUTDEBUG
    NutTraceTcp(stdout, 0);
    NutTraceOs(stdout, 0);
    NutTraceHeap(stdout, 0);
    NutTracePPP(stdout, 0);
#endif

    /*
     * Register Ethernet controller.
     */
    if (NutRegisterDevice(&DEV_ETHER, ETH0_BASE, ETH0_IRQ)) {
        puts("Registering device failed");
    }

    printf("Configure %s...", DEV_ETHER_NAME);
    if (NutNetLoadConfig(DEV_ETHER_NAME)) {
        u_char mac[] = MY_MAC;

        printf("initial boot...");
#ifdef USE_DHCP
		static char eth0IfName[9] = "eth0";
		uint8_t mac_addr[6] = { 0x00, 0x06, 0x98, 0x30, 0x02, 0x76 };
        if (NutDhcpIfConfig(eth0IfName, mac_addr, 0)) 
#endif
        {
            u_long ip_addr = inet_addr(MY_IPADDR);
            u_long ip_mask = inet_addr(MY_IPMASK);
            u_long ip_gate = inet_addr(MY_IPGATE);

            printf("No DHCP...");
            if (NutNetIfConfig(DEV_ETHER_NAME, mac, ip_addr, ip_mask) == 0) {
                /* Without DHCP we had to set the default gateway manually.*/
                if(ip_gate) {
                    printf("hard coded gate...");
                    NutIpRouteAdd(0, 0, ip_gate, &DEV_ETHER);
                }
                puts("OK");
            }
            else {
                puts("failed");
            }
        }
    }
    else {
#ifdef USE_DHCP
        if (NutDhcpIfConfig(DEV_ETHER_NAME, 0, 60000)) {
            puts("failed");
        }
        else {
            puts("OK");
        }
#else
        if (NutNetIfConfig(DEV_ETHER_NAME, 0, 0, confnet.cdn_ip_mask)) {
            puts("failed");
        }
        else {
            puts("OK");
        }
#endif
    }
    printf("%s ready\n", inet_ntoa(confnet.cdn_ip_addr));

#ifdef USE_DISCOVERY
    NutRegisterDiscovery((u_long)-1, 0, DISF_INITAL_ANN);
#endif

#ifdef USE_DATE_AND_TIME
    /* Initialize system clock and calendar. */
    if (InitTimeAndDate() == 0) {
        printf("Local time: ");
        WriteLocalDate(stdout);
        putchar(' ');
        WriteLocalTime(stdout);
        putchar('\n');
        NutHttpSetOptionFlags(NutHttpGetOptionFlags() | HTTP_OF_USE_HOST_TIME | HTTP_OF_USE_FILE_TIME);
    }
#endif

    /*
     * Register our device for the file system.
     */
    NutRegisterDevice(&MY_FSDEV, 0, 0);

#ifdef MY_BLKDEV
    /* Register block device. */
    printf("Registering block device '" MY_BLKDEV_NAME "'...");
    if (NutRegisterDevice(&MY_BLKDEV, 0, 0)) {
        puts("failed");
        for (;;);
    }
    puts("OK");

    /* Mount partition. */
    printf("Mounting block device '" MY_BLKDEV_NAME ":1/" MY_FSDEV_NAME "'...");
    if (_open(MY_BLKDEV_NAME ":1/" MY_FSDEV_NAME, _O_RDWR | _O_BINARY) == -1) {
        puts("failed");
        for (;;);
    }
    puts("OK");
#endif

#ifdef MY_HTTPROOT
    /* Register root path. */
    printf("Registering HTTP root '" MY_HTTPROOT "'...");
    if (NutRegisterHttpRoot(MY_HTTPROOT)) {
        puts("failed");
        for (;;);
    }
    puts("OK");
#endif

    /*
     * Register our CGI sample. This will be called
     * by http://host/cgi-bin/test.cgi?anyparams
     */
    NutRegisterCgi("test.cgi", ShowQuery);

    /*
     * Register some CGI samples, which display interesting
     * system informations.
     */
    NutRegisterCgi("threads.cgi", ShowThreads);
    NutRegisterCgi("timers.cgi", ShowTimers);
    NutRegisterCgi("sockets.cgi", ShowSockets);

#ifdef USE_CGI_PARAMETERS
    /*
     * Finally a CGI example to process a form.
     */
    NutRegisterCgi("form.cgi", ShowForm);
#endif

    /*
     * Protect the cgi-bin directory with
     * user and password.
     */
    NutRegisterAuth("cgi-bin", "root:root");

    /*
     * Register SSI and ASP handler
     */
#ifdef USE_SSI
    NutRegisterSsi();
#endif
#ifdef USE_ASP
    NutRegisterAsp();
    NutRegisterAspCallback(ASPCallback);
#endif
    /*
     * Start twelve server threads.
     */
    for (i = 0; i < HTTPD_MIN_THREADS; i++) {
        StartServiceThread();
    }

    /*
     * We could do something useful here, like serving a watchdog.
     */
    NutThreadSetPriority(254);
    for (;;) {
        NutSleep(60000);
    }
    return 0;
}
