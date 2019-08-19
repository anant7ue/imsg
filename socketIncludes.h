
#include <ctype.h>
#include <sys/time.h>
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


#define IMSG_SERV_PORT      1222
#define IMSG_BACKLOG        5
#define BUFSIZE             256
#define HASHSIZE            256
#define MAX_NUM_CLIENTS     101

#define LANG_ENGLISH    1
#define LANG_HINDI      2
#define LANG_KANNADA    3
#define LANG_SANSKRIT   4
#define LANG_MAX        5 

#define CMD_LOCK_BOOT       0
#define CMD_CHANGEID        1
#define CMD_IMSG_DUP_MINI   2 
#define CMD_RECOMMEND       3
#define CMD_VERIFY_MSG      4
#define CMD_PREF_MEDIA_STREAMS 5
#define CMD_SECURE          6
#define CMD_SIP_SUB         7  /* TODO */
#define CMD_TEXT_MIX        8
#define CMD_STD_LOCAL_MIX   9
#define CMD_PROMO           10
#define CMD_ALERT_OVERRIDE  11 
#define CMD_PIN_SEARCH_BAR  12 /*TODO */
#define CMD_ENCRYPT_DE      13 
#define CMD_HELLO           14
#define CMD_HELP            14
#define CMD_EXIT            21


// call-log timestamp from in/out/missed duration 
// // text-log  timestamp from  type length val
//
