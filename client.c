#include<stdio.h>
#include<stdlib.h>
#include "socketIncludes.h"

char *servAddr="127.0.0.1";
int id = 32;
int RANGE = 21;
int contacts[MAX_NUM_CLIENTS];

main()
{
    int n, sockfd = 0;
    int i, j, count = 0, cmd = -1;
    int oldId, newId;
    char *idptr;
    char msg[BUFSIZE];
    char buf[BUFSIZE];
    struct sockaddr_in cliSock;

    sockfd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);

    if (sockfd == -1) {
        printf("Error opening socket; Exiting...\n");
        return -1;
    }

    bzero( (char *) &cliSock, sizeof(struct sockaddr_in));
    cliSock.sin_family = AF_INET;
    cliSock.sin_port = IMSG_SERV_PORT;
    inet_aton(servAddr, (struct in_addr *) &(cliSock.sin_addr.s_addr) );

    if(connect(sockfd, (const struct sockaddr *) &cliSock, sizeof(struct sockaddr)) < 0)
    {
        printf("could not connect to server %s\n", servAddr);
    }

    memset(msg, BUFSIZE, 0);
    sprintf(msg, "%d says hello world\n", id);
    write(sockfd, msg, strlen(msg)+1);

    for(i = 0; i < MAX_NUM_CLIENTS; i++) {
        n = ( (long) rand() * RANGE) / RAND_MAX;
        if( 2* n > RANGE) {
            contacts[i] = i; 
        }
    }

    count = 0;
    while(cmd != CMD_IMSG) {
        memset(buf, BUFSIZE, '\0');
        n = read(sockfd, buf, BUFSIZE);
        if(n >0) {
            sscanf(buf, "%d ", &cmd);
            printf("Rcvd msg with cmd= %d buf= %s\n buf4= %s\n", cmd, buf, &buf[4]);
            if( cmd == CMD_CHANGEID) {
                sscanf(buf, "%d %d ", &cmd, &count);
                printf("count %d list: %s\n", count, &buf[4]);
                idptr = &buf[4];
                for(i = 0; i < count; i++) {
                    n = 0;
                    oldId = strtol(idptr, &idptr, 10);
                    newId = strtol(idptr, &idptr, 10);
                    for (j = 0; j < MAX_NUM_CLIENTS; j++) {
                        if(contacts[j] == oldId) {
                            contacts[j] = newId;
                            printf("replaced ");
                            n = 1;
                        }
                    }
                    printf("old %d new %d\n", oldId, newId);
                }
            }
        }
    }

    printf("Contacts: \n");
    for(i = 0; i < MAX_NUM_CLIENTS; i++) {
        if((contacts[i] != 0)&&(contacts[i] != id)) {
            printf("%d \t", contacts[i]);
            count++;
        }
    }
    printf("\n Total: %d\n", count);

    printf("Connected to server... please type cmds...\n");

    while(cmd != CMD_EXIT) {
        memset(msg, BUFSIZE, '\0');
        scanf("%d", &cmd );
        printf("rcvd cmd %d\n", cmd);
        if(cmd == CMD_CHANGEID) {
            printf("enter new Id; select contacts to update(1) or All (2)\n");
            scanf("%d %d", &newId, &i);
            sprintf(msg, "%d %d %d %d ",id, cmd, newId, count);
            if (i == 2) {
                for(i = 0; i < MAX_NUM_CLIENTS; i++) {
                    if((contacts[i] != 0)&&(contacts[i] != id)) {
                        sprintf(msg + strlen(msg), "%d ", contacts[i]);
                    }
                }
            }
        } else {
            sprintf(msg, "%d %d cmd sent\n", id, cmd);
        }
        write(sockfd, msg, strlen(msg)+1);
    }

    close(sockfd);
    return -1;
}



