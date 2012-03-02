#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <io.h>

#include <sys/version.h>
#include <sys/confnet.h>
#include <sys/heap.h>
#include <sys/thread.h>
#include <sys/timer.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <netinet/tcp.h>

#include <pro/dhcp.h>
#include <time.h>


#ifndef __PlayStream_H
#define __PlayStream_H

void PlayMp3Stream(FILE *stream, u_long metaint);
int ProcessMetaData(FILE *stream);
FILE *ConnectStation(TCPSOCKET *sock, u_long ip, u_short port, u_long *metaint);
int ConfigureLan(char *devname);

#endif
