
#include<ctype.h>
#include<sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC    02000000
#endif

#define IMSG_SERV_PORT 1221
#define IMSG_BACKLOG 5
#define BUFSIZE 256
#define HASHSIZE 256
#define MAX_NUM_CLIENTS 101

#define L_ENGLISH 1
#define L_HINDI 2
#define L_KANNADA 3
#define LANG_MAX 4

#define CMD_HELLO       0
#define CMD_CHANGEID    1
#define CMD_IMSG        2
#define CMD_COMBINE     3
#define CMD_PREF_MEDIA_STREAMS 4
#define CMD_VERIFY_MSG  5
#define CMD_SECURE      6
#define CMD_SIP_MISS_SUB 7
#define CMD_TEXT_MIX    8
#define CMD_LOAD_ITEM   9
#define CMD_EXIT        10


// call-log timestamp from in/out/missed duration 
// text-log  timestamp from  type length val

