
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

#define IMSG_SERV_PORT 1221
#define IMSG_BACKLOG 5
#define BUFSIZE 256
#define MAX_NUM_CLIENTS 101

#define CMD_HELLO 0
#define CMD_CHANGEID 1
#define CMD_IMSG 2
#define CMD_EXIT 3


