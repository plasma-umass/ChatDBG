/* Copyright (c) 1996, 1999, 2001 Thorsten Kukuk
   Author: Thorsten Kukuk <kukuk@suse.de>

   The YP Server is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   The YP Server is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with the YP Server; see the file COPYING. If
   not, write to the Free Software Foundation, Inc., 675 Mass Ave,
   Cambridge, MA 02139, USA. */

#define _GNU_SOURCE

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "mknetid.h"

#define TABLESIZE 997 /*Should be a prime */

typedef struct hash_liste {
  char *key;
  char *val;
  struct hash_liste *next;
} hash_liste_t;

static
char *xstrtok (char *cp, int delim)
{
    static char *str = NULL;

    if (cp)
        str = cp;

    if (*str == '\0')
        return NULL;

    cp = str;

    if (delim == ' ')
       while (*str && (!isspace(*str)))
          str++;
    else
       while (*str && *str != delim)
          str++;

    if (*str)
        *str++ = '\0';

    return cp;
}

static inline void *xmalloc(unsigned long size)
{
  void *ptr;

  ptr = malloc(size);

  if(ptr == NULL)
    {
      fprintf(stderr,"ERROR: out of memory!\n");
      exit(1);
    }

  return ptr;
}

static hash_liste_t* user_liste[TABLESIZE];
static hash_liste_t* host_liste[TABLESIZE];
static int first = 1;
static char uid_liste[65535];

static void
init_table(void)
{
  first = 0;

  memset(user_liste, 0, sizeof(user_liste));
  memset(host_liste, 0, sizeof(host_liste));
  memset(uid_liste, 0, sizeof(uid_liste));
}

int insert_user(const char *key, const char *domain,
		const char *uid, const char *gid )
{
  long hkey, id;
  size_t i;

  if(first) init_table();

  id = atol(uid);

  if(id > 65534)
    return -2;

  if(uid_liste[id] == 1)
    return -1;
  else
    uid_liste[id] = 1;

  hkey = 0;
  for(i = 0; i < strlen(key); i++)
    hkey = (256*hkey +key[i]) % TABLESIZE;

  if(user_liste[hkey] != NULL)
    {
      hash_liste_t *work, *ptr;

      work = user_liste[hkey]->next;
      ptr = user_liste[hkey];

      while(work != NULL)
	{
	  ptr = work;
	  work = work->next;
	}

      ptr->next = xmalloc(sizeof(hash_liste_t));
      work = ptr->next;
      work->next = NULL;
      work->key = xmalloc(strlen(key)+1);
      strcpy(work->key,key);
      work->val = xmalloc(strlen(domain)+2*strlen(uid)+strlen(gid)+100);
      sprintf(work->val,"unix.%s@%s\t%s:%s",uid,domain,uid,gid);
    }
  else
    {
      user_liste[hkey] = xmalloc(sizeof(hash_liste_t));
      user_liste[hkey]->key = xmalloc(strlen(key)+1);
      strcpy(user_liste[hkey]->key,key);
      user_liste[hkey]->next = NULL;
      user_liste[hkey]->val = xmalloc(strlen(domain)+2*strlen(uid)+strlen(gid)+10);
      sprintf(user_liste[hkey]->val,"unix.%s@%s\t%s:%s",uid,domain,uid,gid);
    }
  return 0;
}

int add_group(const char *key, const char *grp)
{
  hash_liste_t *work;
  long hkey;
  size_t i;

  if(first) init_table();

  hkey = 0;
  for(i = 0; i < strlen(key); i++)
    hkey = (256*hkey +key[i]) % TABLESIZE;

  if(user_liste[hkey] == NULL)
    return -1;
  else
    if(strcmp(user_liste[hkey]->key,key)!=0)
      {
	if(user_liste[hkey]->next == NULL)
	  return -1;
	else
	  {
	    work=user_liste[hkey]->next;
	    while(work != NULL)
	      if(strcmp(work->key,key)==0)
		break;
	      else
		work = work->next;
	    if(work == NULL)
	      return -1;
	  }
      }
    else
      work = user_liste[hkey];

  if(strcmp(key,work->key)==0)
    {
      char *ptr, *tmp;

      tmp = strdup(work->val);
      ptr = xstrtok(tmp,':');

      while((ptr = xstrtok(NULL,','))!= NULL)
	if(strcmp(ptr,grp)==0) return 0;

      ptr = xmalloc(strlen(work->val)+strlen(grp)+5);
      strcpy(ptr,work->val);
      strcat(ptr,",");
      strcat(ptr,grp);
      free(work->val);
      work->val = ptr;

      return 0;
    }

  return -1;
}

int insert_host(const char *host, const char *domain)
{
  long hkey;
  size_t i;

  if(first) init_table();

  hkey = 0;
  for(i = 0; i < strlen(host); i++)
    hkey = (256*hkey +host[i]) % TABLESIZE;

  if(host_liste[hkey] != NULL)
    {
      if(strcmp(host_liste[hkey]->key,host)==0)
	return -1;
      else
	{
	  hash_liste_t *work, *ptr;

	  work=host_liste[hkey]->next;
	  ptr = host_liste[hkey];

	  while(work != NULL)
	    if(strcmp(work->key,host)==0)
	      return -1;
	    else
	      {
		ptr = work;
		work = work->next;
	      }

	  ptr->next = xmalloc(sizeof(hash_liste_t));
	  work = ptr->next;
	  work->next = NULL;
	  work->key = strdup(host);
	  work->val = xmalloc(strlen(host)*2+strlen(domain)+20);
	  sprintf(work->val,"unix.%s@%s\t0:%s",host,domain,host);
	}
    }
  else
    {
      host_liste[hkey] = xmalloc(sizeof(hash_liste_t));
      host_liste[hkey]->key = strdup(host);
      host_liste[hkey]->next = NULL;
      host_liste[hkey]->val = xmalloc(strlen(host)*2+strlen(domain)+20);
      sprintf(host_liste[hkey]->val,"unix.%s@%s\t0:%s",host,domain,host);
    }
  return 0;
}

void print_table()
{
  hash_liste_t *work;
  unsigned long i;

  for(i = 0; i < TABLESIZE; i++)
    {
      work = user_liste[i];
      while(work != NULL)
	{
	  printf("%s\n",work->val);
	  work = work->next;
	}
    }

  for(i = 0; i < TABLESIZE; i++)
    {
      work = host_liste[i];
      while(work != NULL)
	{
	  printf("%s\n",work->val);
	  work = work->next;
	}
    }
}
