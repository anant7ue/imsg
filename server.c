#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "socketIncludes.h"
#include <pthread.h>

#define MAX_NUM_THREADS   4 //MAX_NUM_CLIENTS 
#define RECOMMEND_SIZE    3
#define NOISE_COUNT       5
#define LIKE_FACTOR       2
#define FPATH             "secure.txt"

pthread_mutex_t pmtex[MAX_NUM_THREADS];
pthread_cond_t pcond[MAX_NUM_THREADS];

int  assigned[MAX_NUM_THREADS];
char imei[BUFSIZE], serial[BUFSIZE];
int  lastClean;
int  CLEAN_INTERVAL = 10;

char *counter[LANG_MAX] = {
  "",
  "one two three four five six seven eight nine ten",
  "ek do teen char panch cheh saat aath nau das",
  "ondhu eradu mooru nalku aidhu aaru elu entu ombatthu hattu",
  "ekah dwau trini chatvari panch shashtha saptam ashtam navam dasham",
};

struct hashEntry
{
  int hashVal;
  int origin;
  int timestamp;
  char *msgRef[LANG_MAX];
  char msg[BUFSIZE];
  struct hashEntry *next;
} hTable[HASHSIZE];

struct updateT
{
  int cmd;
  int oldId;
  int newId;
  char msg[BUFSIZE];
  struct updateT *next;
} updates[MAX_NUM_CLIENTS];

int
getHash (char *buf, int clean)
{
  int sig = 0, i = 0;
  for (i = 0; buf[i] != '\0'; i++)
    {
      if (clean == 1)
	{
	  if (isspace (buf[i]))
	    {
	      // check for 'a', 'an', 'the' or 'aaaa' or 'zzz' later
	      // also linewise hash and concatenate later
	      continue;
	    }
	}
      sig += (int) buf[i];
    }
  return sig;
}

void
purgeHash ()
{
  int tnow, i = 0;
  int diff = 0;
  struct timeval tv;
  gettimeofday (&tv, NULL);
  tnow = tv.tv_sec;

  struct hashEntry *hashPtr = NULL;
  for (i = 0; i < HASHSIZE; i++)
    {
      hashPtr = &hTable[i];
      while (hashPtr)
	{
	  if ((hashPtr->hashVal != 0)
	      && (tnow - hashPtr->timestamp > CLEAN_INTERVAL))
	    {
	      printf ("cleaning entry with tstamp %d from %d\n",
		      hashPtr->timestamp, hashPtr->origin);
	      hashPtr->hashVal = 0;
	    }
	  hashPtr = hashPtr->next;
	}
    }
}

void *
serveClient (void *arg)
{
  int n = 1, cmd = 0;
  int sockFd = 0;
  char buf[BUFSIZE], nameBuf[BUFSIZE];
  char oneChangeBuf[BUFSIZE];
  char cmdImsg[] = "2 ";
  /* CMD_IMSG */
  char changeInitBuf[] = "1 2 11 21 17 42";
  /* CMD_CHANGEID COUNT OLD1 NEW1 OLD2 NEW2 */
  char imsgBufFmt[] =
    "2 2 253 1 1 1 1 %d 2 1 32 abcdefghijklmnopqrstuvwxyz3456789";
  char imsg2[] = "abcdefghijklmnopqrstuvwxyz3456789";
  char imsgBuf[BUFSIZE] = { };
  char imsgBuf2[] =
    "2 2 682 1 1 1 1 795 2 1 92 ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz3456789abcdefghijklmnopqrstuvwxyz3456789";
  //char imsgBuf[] ="2 2 (213 (1 1) 1 1) (352 (2 1) 32 abcdefghijklmnopqrstuvwxyz3456789)"; 
  /* CMD_IMSG COUNT ID1 TLV1 ID2 TLV2 T=(mediaType convGroupId) */
  /* sample values */
  int cid, tid = *((int *) arg);
  int count = 0, init = 1;
  int langPref2, langPref, sig, index, hashFound = 0, newId, oldId;
  char *idPtr = NULL;
  struct hashEntry *hashPtr = NULL;
  struct updateT *node = NULL;
  struct timeval tv;

  char bufCall[BUFSIZE] = { }, fName[BUFSIZE] = { };
  char bufText[BUFSIZE] = { };
  int countText, tText, tCall, countCall, numText, numCall, len, lenCall,
    lenText;
  char *bufOut = NULL;
  FILE *fptr, *fptrCall;

  int numViews, numUp, numDown;
  int i, j, bestRated[RECOMMEND_SIZE] = { };

  gettimeofday (&tv, NULL);
  lastClean = tv.tv_sec;
  for (i = 0; i < HASHSIZE; i++)
    {
      hTable[i].origin = 0;
      hTable[i].hashVal = 0;
      hTable[i].msgRef[0] = NULL;
      hTable[i].msgRef[1] = NULL;
      hTable[i].msgRef[2] = NULL;
      hTable[i].msgRef[3] = NULL;
      hTable[i].next = NULL;
    }

  printf ("\thello world from spawned %d ptr %x\n", tid, arg);
  printf ("hash for A..Za..z3..9a..z3..89 = %d\n",
	  getHash
	  ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz3456789abcdefghijklmnopqrstuvwxyz3456789",
	   0));
  while (1)
    {
      cmd = CMD_HELLO;
      init = 1;
      pthread_mutex_lock (&pmtex[tid]);
      printf ("\t %d mutex locked, waiting for cond\n", tid);
      pthread_cond_wait (&pcond[tid], &pmtex[tid]);

      sockFd = assigned[tid];
      printf ("\trecvd by thread tid %d sock %d\n", tid, sockFd);

      while (!((n == 0) || (cmd == CMD_EXIT)))
	{
	  memset (buf, '\0', BUFSIZE);
	  n = read (sockFd, buf, BUFSIZE);
	  if (n > 0 || cmd == CMD_HELLO)
	    {
	      printf ("\tbuf= %s\n", buf);
	      sscanf (buf, "%d %d", &cid, &cmd);
	      if (cid > MAX_NUM_CLIENTS)
		{
		  printf
		    ("Error: client id %d exceeds range; closing connection\n",
		     cid);
		  n = 0;
		}
	      printf ("\tnew cid= %d cmd= %d numread= %d\n", cid, cmd, n);

	      if (init == 1)
		{
		  init = 2;
		  node = &updates[cid];
		  if (!node->next)
		    {
		      write (sockFd, changeInitBuf, sizeof (changeInitBuf) + 1);
		      sleep (1);
		    }
		  else
		    {
		      node = node->next;
		      if (node->cmd == CMD_CHANGEID)
			{
			  /* only 1 change id to enque */
			  sprintf (oneChangeBuf, "%d 1 %d %d", node->cmd,
				   node->oldId, node->newId);
			}
		      write (sockFd, oneChangeBuf, sizeof (oneChangeBuf) + 1);
		      sleep (1);
		    }
		  sprintf (imsgBuf, imsgBufFmt, getHash (imsg2, 1));
		  write (sockFd, imsgBuf, sizeof (imsgBuf) + 1);
		  write (sockFd, cmdImsg, sizeof (cmdImsg) + 1);
		  printf ("\tWriting to sockFd %d buf: %s\n", sockFd, cmdImsg);
		}
	      else if (cmd == CMD_CHANGEID)
		{
		  idPtr = &buf[4];
		  oldId = cid;
		  newId = strtol (idPtr, &idPtr, 10);
		  count = strtol (idPtr, &idPtr, 10);
		  printf (" queuing change id from %d to %d for %d\n", oldId,
			  newId, count);
		  for (i = 0; i < count; i++)
		    {
		      cid = strtol (idPtr, &idPtr, 10);
		      if (cid > MAX_NUM_CLIENTS)
			{
			  printf (" cid %d beyond range\n", cid);
			}
		      node = &updates[cid];
		      while (node->next != NULL)
			{
			  node = node->next;
			}
		      node->next =
			(struct updateT *) malloc (sizeof (struct updateT));
		      node = node->next;
		      node->next = NULL;
		      node->cmd = CMD_CHANGEID;
		      node->oldId = oldId;
		      node->newId = newId;
		      printf ("queued at %d\t", cid);
		    }
		}
	      else if (cmd == CMD_SECURE)
		{
		  fptr = fopen (FPATH, "a+");
		  if (fptr == NULL)
		    {
		      printf ("error opening file %s", FPATH);
		    }
		  else
		    {
		      fprintf (fptr, "%d %s", cid, &buf[4]);
		      printf ("from=%d data=%s hash=%d", cid, &buf[4],
			      getHash (&buf[4], 1));
		      sig = getHash (&buf[4], 1);
		      hashPtr = &hTable[sig % HASHSIZE];
		      gettimeofday (&tv, NULL);
		      if (tv.tv_sec > lastClean + CLEAN_INTERVAL)
			{
			  printf ("cleaning after long time\n");
			  purgeHash ();
			  lastClean = tv.tv_sec;
			}
		      hashFound = 0;
		      while (hashPtr)
			{
			  if (hTable[sig % HASHSIZE].hashVal == 0)
			    {
			      break;
			    }
			  if (hashPtr->hashVal == sig)
			    {
			      hashFound = 1;
			      printf
				("Found msg with hashVal %d at %dorigin sent by %d\n",
				 sig, (sig % HASHSIZE), hashPtr->origin);
			      hashPtr->timestamp = tv.tv_sec;
			      printf ("updated tstamp to %ld\n", tv.tv_sec);
			      break;
			    }
			  hashPtr = hashPtr->next;
			}
		      if (hashFound == 0)
			{
			  hashPtr = &hTable[sig % HASHSIZE];
			  while (hashPtr->hashVal != 0)
			    {
			      if (!hashPtr->next)
				{
				  hashPtr->next = (struct hashEntry *)
				    malloc (sizeof (struct hashEntry));
				  hashPtr->next->hashVal = 0;
				}
			      hashPtr = hashPtr->next;
			    }
			  hashPtr->hashVal = sig;
			  hashPtr->origin = cid;
			  if (init == 2)
			    {
			      hashPtr->msgRef[L_ENGLISH] = counter[L_ENGLISH];
			      hashPtr->msgRef[L_HINDI] = counter[L_HINDI];
			      hashPtr->msgRef[L_KANNADA] = counter[L_KANNADA];
			      printf (" msg = %s sz %d \n",
				      hashPtr->msgRef[L_ENGLISH],
				      strlen (hashPtr->msgRef[L_ENGLISH]));
			    }
			  // add message copy and/or TLV reference
			  gettimeofday (&tv, NULL);
			  hashPtr->timestamp = tv.tv_sec;
			  printf ("set tstamp to %ld\n", tv.tv_sec);
			}
		      fsync (fileno (fptr));
		      fclose (fptr);
		      fflush (0);
		    }
		}
	      else if (cmd == CMD_PREF_MEDIA_STREAMS)
		{
		  idPtr = &buf[4];
		  sig = strtol (idPtr, &idPtr, 10);
		  langPref = strtol (idPtr, &idPtr, 10);
		  langPref2 = strtol (idPtr, &idPtr, 10);
		  hashPtr = &hTable[sig % HASHSIZE];
		  hashFound = 0;
		  while (hashPtr)
		    {
		      if (hTable[sig % HASHSIZE].hashVal == 0)
			{
			  break;
			}
		      if (hashPtr->hashVal == sig)
			{
			  hashFound = 1;
			  printf
			    ("Found msg with hashVal %d origin sent by %d\n",
			     sig, hashPtr->origin);
			  break;
			}
		      hashPtr = hashPtr->next;
		    }
		  if (hashPtr->msgRef[langPref])
		    {
		    }
		  else if (hashPtr->msgRef[langPref2])
		    {
		      langPref = langPref2;
		    }
		  else
		    {
		      langPref = 0;
		    }
		  printf (" msg = %s\n", hashPtr->msgRef[langPref]);
		  n =
		    write (sockFd, hashPtr->msgRef[langPref],
			   strlen (hashPtr->msgRef[langPref]) + 1);
		  fflush (0);
		  printf ("sig %d lang %d sz %d n %d hval %d\n", sig,
			  langPref, strlen (hashPtr->msgRef[langPref]), n,
			  hashPtr->hashVal);
		}
	      else if (cmd == CMD_TEXT_MIX)
		{
		  memset (imsgBuf, '\0', BUFSIZE);
		  memset (bufText, '\0', BUFSIZE);
		  memset (bufCall, '\0', BUFSIZE);
		  sprintf (fName, "text_%d_log", cid);
		  fptr = fopen (fName, "r");
		  printf (" fname %s fptr %x\n", fName, fptr);
		  sprintf (fName, "call_%d_log", cid);
		  fptrCall = fopen (fName, "r");
		  if ((fptr == NULL) || (fptrCall == NULL))
		    {
		      printf ("Missing data; nothing to combine");
		      continue;
		    }
		  printf (" fname %s fptr %x\n", fName, fptr);
		  fscanf (fptr, "%d\n %d %s %d %s", &numText, &tText, nameBuf,
			  &lenText, bufText);
		  fscanf (fptrCall, "%d\n %d %s %d %s", &numCall, &tCall,
			  nameBuf, &lenCall, bufCall);
		  countText = 0;
		  countCall = 0;
		  if (tText < tCall)
		    {
		      j = 1;
		    }
		  else
		    {
		      j = 2;
		    }
		  while (countText + countCall < numText + numCall)
		    {
		      if (j == 1)
			{
			  countText++;
			  sprintf (imsgBuf + strlen (imsgBuf), "%d %s \n",
				   tText, bufText);
			  memset (bufText, '\0', BUFSIZE);
			  if (countText < numText)
			    {
			      fscanf (fptr, "%d %s %d %s\n", &tText, nameBuf,
				      &lenText, bufText);
			      if (tText > tCall)
				{
				  j = 2;
				}
			      else
				{
				  j = 2;
				}
			    }
			}
		      else
			{
			  //  (j == 2) 
			  countCall++;
			  sprintf (imsgBuf + strlen (imsgBuf), "%d %s \n",
				   tCall, bufCall);
			  memset (bufCall, '\0', BUFSIZE);
			  if (countCall < numCall)
			    {
			      fscanf (fptrCall, "%d %s %d %s\n", &tCall,
				      nameBuf, &lenCall, bufCall);
			      if (tText < tCall)
				{
				  j = 1;
				}
			    }
			  else
			    {
			      j = 1;
			    }
			}
		    }
		  printf (" cmd %d respnse %s\n", CMD_TEXT_MIX, imsgBuf);
		  n = write (sockFd, imsgBuf, strlen (imsgBuf) + 1);

		  // filename from cid, fopen
		  // read text from file
		  // tstamp from len val ---- FMT
		  // sort tstamps
		  // write
		}
	      else if (cmd == CMD_STD_LOCAL_MIX)
		{
		  memset (imsgBuf, '\0', BUFSIZE);
		  idPtr = &buf[4];
		  oldId = cid;
		  newId = strtol (idPtr, &idPtr, 10);
		  printf (" queuing change id from %d to %d for %d\n", oldId,
			  newId, count);
		  if (init == 2)
		    {
		      init = 3;
		      sprintf (imsgBuf, "a b 12321 %s \n", &buf[8]);
		    }
		  else
		    {
		      init = 2;
		      sprintf (imsgBuf, "c d 121 %s \n", &buf[8]);
		    }
		  if (newId == cid)
		    {
		      n = write (sockFd, imsgBuf, strlen (imsgBuf) + 1);
		    }
		  // read newId, msg
		  // compose msg from file + macro
		  // if newId == cid, write else enQ
		}
	      else if (cmd == CMD_RECOMMEND)
		{
		  // read file for list of items and rating 
		  memset (imsgBuf, '\0', BUFSIZE);
		  fptr = fopen ("ratings.txt", "r");
		  fscanf (fptr, "%d", &numText);
		  for (j = 0, i = 0; i < numText; i++)
		    {
		      if (i <= RECOMMEND_SIZE)
			bestRated[i] = 0;
		      fscanf (fptr, "%d %d %d", &numViews, &numUp, &numDown);
		      printf ("%d %d %d\n", numViews, numUp, numDown);
		      if ((numUp > NOISE_COUNT)
			  && (numUp > LIKE_FACTOR * numDown))
			{
			  printf ("\t%d %d %d\n", numViews, numUp, numDown);
			  bestRated[j++] = i;
			  if (j == RECOMMEND_SIZE)
			    break;
			}
		    }
		  for (i = 0; i < RECOMMEND_SIZE; i++)
		    {
		      if (bestRated[i] == 0)
			break;
		      sprintf (imsgBuf + strlen (imsgBuf), "%d \t",
			       bestRated[i]);
		      printf ("%d \t", bestRated[i]);
		      printf ("%s \n", imsgBuf);
		    }
		  printf ("%s \n", imsgBuf);
		  n = write (sockFd, imsgBuf, strlen (imsgBuf) + 1);
		  // item_id num_views up down ---- FMT

		}
	      else if (cmd == CMD_LOCK_BOOT)
		{
		  memset (imsgBuf, '\0', BUFSIZE);
		  fptr = fopen ("keyPart2", "w+");
		  fprintf (fptr, "WXYZ");
		  fclose (fptr);
		  system
		    ("sshpass -p ubuntu scp keyPart2 ubuntu@127.0.0.1:~ubuntu/imsg/key");
		  sprintf (imsgBuf, "ABCD");
		  n = write (sockFd, imsgBuf, strlen (imsgBuf) + 1);
		}
	      else if (cmd == CMD_IMSG)
		{
		}
	      //} else if(cmd == CMD_SECURE) {
	    }
	}
      fflush (0);
      assigned[tid] = 0;
      sleep (1);
      printf ("\t %d mutex unlocked\n", tid);
      pthread_mutex_unlock (&pmtex[tid]);
    }

  pthread_exit (0);
  return NULL;
}

int
getFreeThread ()
{
  int i, numConn = 0;

  printf ("freeChk\n");
  for (i = 0; i < MAX_NUM_THREADS; i++)
    {
      printf ("%d-busy %d\n", i, assigned[i]);
      if (assigned[i] == 0)
	{
	  return i;
	}
    }
  return -1;
}

main ()
{
  int sockfd, newFd = 0;
  socklen_t sockSz = sizeof (struct sockaddr);
  struct sockaddr_in servSock;
  int n = 4, cmd = 0;
  int numConn = 0;
  int *id;
  pthread_t tid;
  pthread_attr_t thrdAttr;

  pthread_attr_init (&thrdAttr);
  id = (int *) malloc (MAX_NUM_THREADS * sizeof (int));
  for (n = 0; n < MAX_NUM_CLIENTS; n++)
    {
      updates[n].next = NULL;
    }

  for (n = 0; n < MAX_NUM_THREADS; n++)
    {
      id[n] = n;
      tid = n;
      printf ("created thread pool id %d ptr %x\n", id[n], &id[n]);
      pthread_create (&tid, &thrdAttr, serveClient, (void *) (&id[n]));
      pthread_mutex_init (&pmtex[n], NULL);
      pthread_cond_init (&pcond[n], NULL);
    }
  /*
     for(n = 0; n < MAX_NUM_THREADS; n++) 
     {
     cmd = *(int *) (&id[n]);
     printf("n= %d val = %d\n",n,  cmd);
     } */

  sockfd = socket (AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

  if (sockfd == -1)
    {
      printf ("Error opening socket; Exiting...\n");
      return -1;
    }

  servSock.sin_family = AF_INET;
  servSock.sin_addr.s_addr = INADDR_ANY;
  servSock.sin_port = IMSG_SERV_PORT;

  if (bind (sockfd, (struct sockaddr *) &servSock, sockSz) == -1)
    {
      printf ("failed to bind... exiting...\n");
      return -1;
    }

  if (listen (sockfd, IMSG_BACKLOG) == -1)
    {
      printf ("failed to listen... exiting...\n");
      return -1;
    }

  printf ("\n waiting for client conn \n");
  while (numConn < MAX_NUM_THREADS)
    {
      newFd = accept (sockfd, (struct sockaddr *) &servSock, &sockSz);
      if (newFd < 0)
	{
	  printf ("accept failed, newfd %d < 0\n", newFd);
	}
      tid = -1;
      while (tid == -1)
	{
	  tid = getFreeThread ();
	}
      pthread_mutex_lock (&pmtex[tid]);
      assigned[tid] = newFd;
      n = pthread_cond_signal (&pcond[tid]);
      pthread_mutex_unlock (&pmtex[tid]);
      printf ("signaling thread id %d sock %d retval=%d \n", tid, newFd, n);
      numConn++;

    }
  printf ("Exiting normally... 0\n");
  close (sockfd);
  return 0;
}

