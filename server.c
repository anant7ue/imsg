#include<stdio.h>
#include<stdlib.h>
#include "socketIncludes.h"
#include<pthread.h>

#define MAX_NUM_THREADS 4 //MAX_NUM_CLIENTS 

pthread_mutex_t pmtex[MAX_NUM_THREADS];
pthread_cond_t pcond[MAX_NUM_THREADS];
int assigned[MAX_NUM_THREADS];

struct updateT {
    int cmd;
    int oldId;
    int newId;
    char msg[BUFSIZE];
    struct updateT *next;
} updates[MAX_NUM_CLIENTS];

void* serveClient(void *arg)
{
    int n = 1, cmd = 0; 
    int sockFd = 0;
    char buf[BUFSIZE];
    char wbuf[BUFSIZE];
    char wbuf1[]="2 ";
    char wbuf2[]="1 2 11 21 17 42";
    int cid, tid = *((int *) arg);
    int count = 0, init = 1;
    int i, newId, oldId;
    char* idPtr = NULL;
    struct updateT* node = NULL;

    printf("\thello world from spawned %d ptr %x\n", tid, arg); 
    while(1) {
        cmd = CMD_HELLO;
        init = 1;
        pthread_mutex_lock(&pmtex[tid]);
        printf("\t %d mutex locked, waiting for cond\n", tid); 
        pthread_cond_wait(&pcond[tid], &pmtex[tid]);

        sockFd = assigned[tid];
        printf("\trecvd by thread tid %d sock %d\n", tid, sockFd);

        while (!((n == 0) ||(cmd == CMD_EXIT))) {
            memset(buf, '\0', BUFSIZE);
            n = read(sockFd, buf, BUFSIZE);
            if( n > 0 || cmd == CMD_HELLO) {
                printf("\tbuf= %s\n", buf);
                sscanf(buf, "%d %d", &cid, &cmd);
                if(cid > MAX_NUM_CLIENTS) {
                    printf("Error: client id %d exceeds range; closing connection\n", cid);
                    n = 0;
                }
                printf("\tnew cid= %d cmd= %d numread= %d\n", cid, cmd, n );

                if (init == 1) {
                    init = 0;
                    node = &updates[cid];
                    if(!node->next) {
                        write(sockFd, wbuf2, sizeof(wbuf2)+1);
                        sleep(1);
                        write(sockFd, wbuf1, sizeof(wbuf1)+1);
                    } else {
                        node = node->next;
                        if(node->cmd == CMD_CHANGEID) {
                            sprintf(wbuf, "%d 1 %d %d", node->cmd, node->oldId, node->newId);
                        }
                        write(sockFd, wbuf, sizeof(wbuf)+1);
                        sleep(1);
                        write(sockFd, wbuf1, sizeof(wbuf1)+1);
                    }
                    printf("\tWriting to sockFd %d buf: %s\n", sockFd, wbuf1 );
                } else if(cmd == CMD_CHANGEID) {
                    idPtr = &buf[4];
                    oldId = cid;
                    newId = strtol(idPtr, &idPtr, 10);
                    count = strtol(idPtr, &idPtr, 10);
                    printf(" queuing change id to %d for %d\n", newId, count);
                    for (i = 0; i < count; i ++) {
                        cid = strtol(idPtr, &idPtr, 10);
                        if (cid > MAX_NUM_CLIENTS) {
                            printf(" cid %d beyond range\n", cid);
                        }
                        node = &updates[cid];
                        while(node->next != NULL) {
                            node = node->next;
                        }
                        node->next = (struct updateT *)malloc(sizeof(struct updateT));
                        node = node->next;
                        node->next = NULL;
                        node->cmd = CMD_CHANGEID;
                        node->oldId = oldId;
                        node->newId = newId;
                        printf("queued at %d\t", cid);
                    }
                }
            }
        }
        printf("\t %d mutex unlocked\n", tid); 
        fflush(0);
        sleep(1);
        assigned[tid] = 0;
        pthread_mutex_unlock(&pmtex[tid]);
    }

    pthread_exit(0);
    return NULL;
}

int getFreeThread()
{
    int i, numConn = 0;

    printf("freeChk\n");
    for (i=0; i < MAX_NUM_THREADS; i++)
    {
        printf("%d-busy %d\n",i, assigned[i]);
        if(assigned[i] == 0)
        {
            return i;
        }
    }
return -1;
}

main()
{
    int sockfd, newFd = 0;
    socklen_t sockSz = sizeof(struct sockaddr);
    struct sockaddr_in servSock;
    int n = 4, cmd = 0;
    int numConn = 0;
    int *id; 
    pthread_t tid; 
    pthread_attr_t thrdAttr;

    pthread_attr_init(&thrdAttr);
    id = (int *) malloc(MAX_NUM_THREADS*sizeof(int));
    for(n = 0; n < MAX_NUM_CLIENTS; n++) {
        updates[n].next = NULL;
    }

    for(n = 0; n < MAX_NUM_THREADS; n++) 
    {
        id[n] = n;
        tid = n;
        printf("created thread pool id %d ptr %x\n", id[n], &id[n]);
        pthread_create(&tid, &thrdAttr, serveClient, (void *) (&id[n]));
        pthread_mutex_init(&pmtex[n], NULL);
        pthread_cond_init(&pcond[n], NULL);
    }
    /*
       for(n = 0; n < MAX_NUM_THREADS; n++) 
       {
       cmd = *(int *) (&id[n]);
       printf("n= %d val = %d\n",n,  cmd);
       }*/

    sockfd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);

    if (sockfd == -1) {
        printf("Error opening socket; Exiting...\n");
        return -1;
    }

    servSock.sin_family = AF_INET;
    servSock.sin_addr.s_addr = INADDR_ANY;
    servSock.sin_port = IMSG_SERV_PORT;

    if (bind(sockfd, (struct sockaddr *) &servSock, sockSz) == -1)
    {
        printf("failed to bind... exiting...\n");
        return -1;
    }

    if (listen(sockfd, IMSG_BACKLOG) == -1)
    {
        printf("failed to listen... exiting...\n");
        return -1;
    }

    printf("\n waiting for client conn \n");
    while(numConn < MAX_NUM_THREADS) {
        newFd = accept(sockfd, (struct sockaddr *) &servSock, &sockSz);
        if(newFd < 0) {
            printf("accept failed, newfd %d < 0\n", newFd);
        }
        tid = -1;
        while (tid == -1)
        {
            tid = getFreeThread();
        }
        pthread_mutex_lock(&pmtex[tid]);
        assigned[tid] = newFd;
        n = pthread_cond_signal(&pcond[tid]);
        pthread_mutex_unlock(&pmtex[tid]);
        printf("signaling thread id %d sock %d retval=%d \n", tid, newFd, n);
        numConn++;

    }
    printf("Exiting normally... 0\n");
    close(sockfd);
    return 0;
}

