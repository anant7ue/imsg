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

struct node
{
  int id;
  char msg[BUFSIZE];
  struct node *next;
} head;

int main()
{
  int len, confirm = 0, n, sockfd = 0;
  int miniFetch = 0, alertOverride = 0, mcast, i, j, count = 0, cmd = -1;
  int oldId, destId, newId, fillIndicator;
  int msgCrc, msgIdVerify = 0, videoOff = 0, videoLang2 = 0, videoLang = 1;
  char *idptr;
  char inChar, msg[BUFSIZE] = { };
  char promo[BUFSIZE], localData[BUFSIZE] = { };
  char char1, char2, buf[BUFSIZE] = { };
  struct sockaddr_in cliSock;
  struct node *ptr = NULL;
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
  while (cmd != CMD_IMSG_DUP_MINI)
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
	  else if (cmd == CMD_IMSG_DUP_MINI)
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

  printf ("\n Please set alert override pref (0 or 1): \t");
  scanf ("%d", &alertOverride);
  printf ("\n Please set mini Fetch pref (0 or 1): \t");
  scanf ("%d", &miniFetch);
  printf
    ("Connected to server... please type cmds... %d for help %d to quit \n",
     CMD_HELP, CMD_EXIT);

  while (cmd != CMD_EXIT)
    {
      memset (msg, BUFSIZE, '\0');
      printf ("awaiting cmd...");
      scanf ("%d", &cmd);
      printf ("rcvd cmd %d\n", cmd);

      if (cmd == CMD_HELLO)
	{
	  printf
	    ("Cmd help: CMD_LOCKBOOT 0 \t CMD_CHANGEID 1 \t CMD_IMSG_DUP_MINI 2 \t CMD_RECOMMEND 3 \n"
	     "\t CMD_VERIFY_MSG 4 \t CMD_SIP_SUB 7 \t PREF_MEDIA_STREAMS 5 \t CMD_SECURE 6 \n"
	     "\t CMD_TEXT_MIX 8 \t CMD_STD_LOCAL_MIX 9 \t CMD_PROMO 10 \n"
	     "\t CMD_ALERT_OVERRIDE 11 \t CMD_ENCRYPT_DE 13 \t CMD_HELP 14 \n");
	  // #define CMD_PIN_SEARCH_BAR  12 /*TODO */

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
      else if (cmd == CMD_ALERT_OVERRIDE)
	{
	  printf ("Please use cmd IMSG_DUP_MINI \n");
	  continue;

	}
      else if (cmd == CMD_IMSG_DUP_MINI)
	{
	  sprintf (msg, "%d %d %d %s", myId, cmd, myId, "hi");

	}
      else
	{
	  // use separate else if when special msg to be tx
	  // } else if (cmd == CMD_RECOMMEND) {
	  // } else if (cmd == CMD_PROMO) {
	  // } else if (cmd == CMD_SIP_SUB) {
	  // } else if (cmd == CMD_ENCRYPT_DE) {
	  // } else if (cmd == CMD_LOCK_BOOT) {
	  // } else if (cmd == CMD_TEXT_MIX) {
	  sprintf (msg, "%d %d cmd sent\n", myId, cmd);
	}
      write (sockfd, msg, strlen (msg) + 1);

      if ((cmd == CMD_PREF_MEDIA_STREAMS) ||
	  (cmd == CMD_TEXT_MIX) ||
	  (cmd == CMD_STD_LOCAL_MIX) ||
	  (cmd == CMD_IMSG_DUP_MINI) ||
	  (cmd == CMD_RECOMMEND) ||
	  (cmd == CMD_PROMO) ||
	  (cmd == CMD_ENCRYPT_DE) || (cmd == CMD_LOCK_BOOT))
	{

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
	  else if (cmd == CMD_ENCRYPT_DE)
	    {
	      printf ("please enter key to decrypt");
	      newId = 0;
	      scanf ("%s", localData);
	      len = strlen (localData);
	      sscanf (buf, "%d %n", &msgCrc, &n);
	      i = n - 1;
	      count = 0;
	      printf ("crc %d i %d encoded %s\n", msgCrc, i, (buf + i));
	      msgIdVerify = 0;
	      for (j = 0; buf[i] != '\0'; count++)
		{
		  sscanf ((buf + i), "%d %n", &newId, &n);
		  i += n;
		  msg[count] = newId;
		  msg[count] = msg[count] ^ localData[j];
		  j++;
		  if (j == len)
		    {
		      j = 0;
		    }
		  msgIdVerify += msg[count];
		}
	      if (msgIdVerify == msgCrc)
		{
		  printf ("\n%s %s idChk=%d\n", localData, msg, msgIdVerify);
		}
	      else
		{
		  printf ("Password incorrect\n");
		}

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
	  else if (cmd == CMD_IMSG_DUP_MINI)
	    {
	      sscanf (buf, "%d %d", &newId, &msgIdVerify);
	      ptr = &head;
	      j = 0;
	      if (ptr->id != 0)
		{
		  while (ptr->next)
		    {
		      if (ptr->id == msgIdVerify)
			{
			  j = 1;
			  printf ("dup msg found %d\n", msgIdVerify);
			}
		      ptr = ptr->next;
		    }
		}
	      if (j == 0)
		{
		  ptr->id = msgIdVerify;
		  ptr->next = (struct node *) malloc (sizeof (struct node));
		  ptr->next->id = 0;
		}
	      if (alertOverride == 1)
		{
		  if (miniFetch == 1)
		    {
		      printf ("rcvd from %d buf %s\n", newId, &buf[5]);
		    }
		  else
		    {
		      printf ("rcvd from %d buf %s\n", newId, buf);
		    }
		}
	    }
	  else if (cmd == CMD_PROMO)
	    {
	      memset (promo, BUFSIZE, 0);
	      memset (msg, BUFSIZE, 0);
	      sscanf (buf, "%s %s", promo, msg);
	      printf ("rcvd promo %s \t data %s\n", promo, msg);
	      printf ("left key to replay, any other key to exit");
	      scanf (" %c", &inChar);
	      printf ("rcvd cmd %d \n", inChar);
	      j = 0;
	      while (inChar == '\033')
		{
		  scanf ("%c", &inChar);
		  j = 1;
		  scanf ("%c", &inChar);
		  switch (inChar)
		    {
		    case 'D':
		      {
			printf ("replaying promo... %s", promo);
			break;
		      }

		    }
		  scanf (" %c", &inChar);
		}

	    }
	  if ((cmd == CMD_RECOMMEND) || (cmd == CMD_TEXT_MIX))
	    {
	      // print
	    }
	  printf ("rcvd response sz %d buf %s\n", n, buf);
	  fflush (0);
	}
    }

  close (sockfd);
  return -1;
}
