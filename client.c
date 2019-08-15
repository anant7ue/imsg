
#include<stdio.h>
#include<stdlib.h>
#include "socketIncludes.h"

char *servAddr="127.0.0.1";
int myId = 21;
char *imei = "861536030196032";
char *serial="DB168091200532";
int RAND_RANGE = 21;
int contacts[MAX_NUM_CLIENTS];
char firstName[MAX_NUM_CLIENTS];
char lastName[MAX_NUM_CLIENTS];

int main()
{
  int confirm = 0, n, sockfd = 0;
  int mcast, i, j, count = 0, cmd = -1;
  int oldId, destId, newId, fillIndicator;
  int msgIdVerify = 0, videoOff = 0, videoLang2 = 0, videoLang = 1;
  char *idptr;
  char msg[BUFSIZE] = { };
  char localData[BUFSIZE] = { };
  char char1, char2, buf[BUFSIZE] = { };
  struct sockaddr_in cliSock;
  FILE *fData;

  sockfd = socket (AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

  if (sockfd == -1)
    {
      printf ("Error opening socket; Exiting...\n");
      return -1;
    }

  bzero ((char *) &cliSock, sizeof (struct sockaddr_in));
  cliSock.sin_family = AF_INET;
  cliSock.sin_port = IMSG_SERV_PORT;
  inet_aton (servAddr, (struct in_addr *) &(cliSock.sin_addr.s_addr));

  if (connect
      (sockfd, (const struct sockaddr *) &cliSock,
       sizeof (struct sockaddr)) < 0)
    {
      printf ("could not connect to server %s\n", servAddr);
    }

  memset (msg, BUFSIZE, 0);
  // mac memset(msg, 0, BUFSIZE);
  sprintf (msg, "%d %d says hello world\n", myId, CMD_HELLO);
  write (sockfd, msg, strlen (msg) + 1);

  for (i = 0; i < MAX_NUM_CLIENTS; i++)
    {
      n = ((long) rand () * RAND_RANGE) / RAND_MAX;
      if (i == myId)
	continue;
      if (2 * n > RAND_RANGE)
	{
	  contacts[i] = i;
	  firstName[i] = 'a' + (i / 26);
	  lastName[i] = 'A' + (i % 26);
	  count++;
	}
    }

  printf ("Contacts before: \n");
  for (i = 0; i < MAX_NUM_CLIENTS; i++)
    {
      if ((contacts[i] != 0) && (contacts[i] != myId))
	{
	  printf ("%d %c %c \t", contacts[i], firstName[i], lastName[i]);
	}
    }
  count = 0;
  while (cmd != CMD_IMSG)
    {
      memset (buf, BUFSIZE, '\0');
      n = read (sockfd, buf, BUFSIZE);
      if (n > 0)
	{
	  sscanf (buf, "%d ", &cmd);
	  printf ("\n Rcvd msg with cmd= %d buf= %s\n", cmd, buf);
	  if (cmd == CMD_CHANGEID)
	    {
	      sscanf (buf, "%d %d ", &cmd, &count);
	      printf ("count %d list: %s\n", count, &buf[4]);
	      idptr = &buf[4];
	      for (i = 0; i < count; i++)
		{
		  n = 0;
		  oldId = strtol (idptr, &idptr, 10);
		  newId = strtol (idptr, &idptr, 10);
		  for (j = 0; j < MAX_NUM_CLIENTS; j++)
		    {
		      if (contacts[j] == oldId)
			{
			  printf
			    ("%c %c updated contact from %d to %d ! Accept_Update (1) or No (0) ?\n",
			     firstName[j], lastName[j], oldId, newId);
			  scanf ("%d", &confirm);
			  if (confirm == 1)
			    {
			      contacts[j] = newId;
			      printf ("replaced ");
			      n = 1;
			    }
			}
		    }
		  printf ("old %d new %d\n", oldId, newId);
		}
	    }
	  else if (cmd == CMD_IMSG)
	    {
	      printf ("imsg archive buf start\n");
	    }
	}
    }

  n = read (sockfd, buf, BUFSIZE);
  printf ("Contacts After: \n");
  for (i = 0; i < MAX_NUM_CLIENTS; i++)
    {
      if ((contacts[i] != 0) && (contacts[i] != myId))
	{
	  printf ("%d \t", contacts[i]);
	  count++;
	}
    }
  printf ("\n Total: %d\n", count);

  printf
    ("Connected to server... please type cmds... 10 for help %d to quit \n",
     CMD_EXIT);

  while (cmd != CMD_EXIT)
    {
      memset (msg, BUFSIZE, '\0');
      printf ("awaiting cmd...");
      scanf ("%d", &cmd);
      printf ("rcvd cmd %d\n", cmd);

      if (cmd == CMD_HELLO)
	{
	  printf
	    ("Cmd help: CMD_LOCKBOOT 0 \t CMD_CHANGEID 1 \t CMD_IMSG 2 \t CMD_RECOMMEND 3 \n"
	     "\t PREF_MEDIA_STREAMS 4 \t CMD_VERIFY_MSG 5 \t CMD_SECURE 6 \t CMD_SIP_SUB 7\n "
	     "\t CMD_TEXT_MIX 8 \t CMD_STD_LOCAL_MIX 9 \t CMD_HELP 10 \n");
	}
      if (cmd == CMD_CHANGEID)
	{
	  printf ("enter new Id; select contacts to update(1) or All (2)\n");
	  scanf ("%d %d", &newId, &mcast);
	  sprintf (msg, "%d %d %d %d ", myId, cmd, newId, count);
	  if (mcast == 2)
	    {
	      for (i = 0; i < MAX_NUM_CLIENTS; i++)
		{
		  if ((contacts[i] != 0) && (contacts[i] != myId))
		    {
		      sprintf (msg + strlen (msg), "%d ", contacts[i]);
		    }
		}
	    }
	}
      else if (cmd == CMD_SECURE)
	{
	  printf ("sending IMEI %s and serial %s\n", imei, serial);
	  sprintf (msg, "%d %d %s %s ", myId, cmd, imei, serial);

	}
      else if (cmd == CMD_PREF_MEDIA_STREAMS)
	{
	  printf
	    ("please sig (//enter multimedia preferences videoOff(0/1) and 2 Langs(1-english 2-hindi 3-kannada ...%d)\n",
	     LANG_MAX);
	  scanf ("%d %d %d", &videoOff, &videoLang, &videoLang2);
	  printf ("sending multimedia preferences\n");
	  sprintf (msg, "%d %d %d %d %d ", myId, cmd, videoOff, videoLang,
		   videoLang2);
	}
      else if (cmd == CMD_VERIFY_MSG)
	{
	  printf ("please enter msg id to verify\n");
	  scanf ("%d ", &msgIdVerify);
	  sprintf (msg, "%d %d %d ", myId, cmd, msgIdVerify);
	}
      else if (cmd == CMD_TEXT_MIX)
	{
	  sprintf (msg, "%d %d ", myId, cmd);
	}
      else if (cmd == CMD_RECOMMEND)
	{
	  sprintf (msg, "%d %d ", myId, cmd);
	}
      else if (cmd == CMD_STD_LOCAL_MIX)
	{
	  memset (msg, BUFSIZE, '\0');
	  printf ("please enter dest_id \n");
	  scanf ("%d", &destId);
	  sprintf (msg, "%d %d %d ", myId, cmd, destId);
	  printf ("%d %d %d sz=%d", myId, cmd, destId, strlen (msg));
	  printf ("please enter  msg \n");
	  scanf ("%s", msg + strlen (msg));
	}
      else if (cmd == CMD_LOCK_BOOT)
	{
	  sprintf (msg, "%d %d ", myId, cmd);
	}
      else if (cmd == CMD_SIP_SUB)
	{
	  sprintf (msg, "%d %d ", myId, cmd);
	}
      else
	{
	  sprintf (msg, "%d %d cmd sent\n", myId, cmd);
	}
      write (sockfd, msg, strlen (msg) + 1);
      if ((cmd == CMD_PREF_MEDIA_STREAMS) || (cmd == CMD_TEXT_MIX)
	  || (cmd == CMD_STD_LOCAL_MIX) || (cmd == CMD_RECOMMEND)
	  || (cmd == CMD_LOCK_BOOT))
	{
	  //n = 0;
	  //while(n == 0) {
	  memset (msg, BUFSIZE, '\0');
	  n = read (sockfd, buf, BUFSIZE);
	  if (cmd == CMD_STD_LOCAL_MIX)
	    {
	      // decode + local file fill 
	      sscanf (buf, "%c %c %d %s", &char1, &char2, &fillIndicator,
		      msg);
	      if (fillIndicator == 12321)
		{
		  printf ("%c%c %s %s\n", char1, char2,
			  "yes!localDetailsHere", msg);
		}
	      else if (fillIndicator == 121)
		{
		  memset (localData, BUFSIZE, 0);
		  fData = fopen ("dataFile.txt", "r");
		  fscanf (fData, "%s", localData);
		  fclose (fData);
		  printf ("%c%c %s %s\n", char1, char2, localData, msg);
		}
	    }
	  if ((cmd == CMD_RECOMMEND) || (cmd == CMD_TEXT_MIX))
	    {
	      // print
	    }
	  else if (cmd == CMD_LOCK_BOOT)
	    {
	      sscanf (buf, "%s", msg);
	      memset (localData, BUFSIZE, 0);
	      fData = fopen ("key", "r");
	      fscanf (fData, "%s", localData);
	      fclose (fData);
	      printf ("%s %s\n", localData, msg);
	    }
	  printf ("rcvd response sz %d buf %s\n", n, buf);
	  fflush (0);
	  //}
	}
    }

  close (sockfd);
  return -1;
}

