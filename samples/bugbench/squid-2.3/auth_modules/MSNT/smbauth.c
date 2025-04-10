
/*
  msntauth

  Modified to act as a Squid authenticator module.
  Removed all Pike stuff.
  Returns OK for a successful authentication, or ERR upon error.

  Antonino Iannella, Camtech SA Pty Ltd
  Mon Apr 10 22:24:26 CST 2000

  Uses code from -
    Andrew Tridgell 1997
    Richard Sharpe 1996
    Bill Welliver 1999

  Released under GNU Public License

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "sitedef.h"

extern void Checkforchange();       /* For signal() to find the function */

/* Main program for simple authentication.
   Reads the denied user file. Sets alarm timer.
   Scans and checks for Squid input, and attempts to validate the user.
*/

int main()
{
  char username[256];
  char password[256];
  char wstr[256];
  struct itimerval TimeOut;

  /* Read denied user file. If it fails there is a serious problem.
     Check syslog messages. Deny all users while in this state.
     The process should then be killed. */

  if (Read_denyusers() == 1)
  {
     while (1)
     {
       fgets(wstr, 255, stdin);
       puts("ERR");
       fflush(stdout);
     }
  }

  /* An alarm timer is used to check the denied user file for changes
     every minute. Reload the file if it has changed. */ 

  TimeOut.it_interval.tv_sec = 60;
  TimeOut.it_interval.tv_usec = 0;
  TimeOut.it_value.tv_sec = 60;
  TimeOut.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &TimeOut, 0);
  signal(SIGALRM, Checkforchange);
  signal(SIGHUP, Checkforchange);

  while (1)
  {
    /* Read whole line from standard input. Terminate on break. */
    if (fgets(wstr, 255, stdin) == NULL)    
       break;

    /* Clear any current settings */
    username[0] = '\0';
    password[0] = '\0';
    sscanf(wstr, "%s %s", username, password);     /* Extract parameters */

    /* Check for invalid or blank entries */
    if ((username[0] == '\0') || (password[0] == '\0'))
  {
       puts("ERR");
       fflush(stdout);
       continue;
    }

    if (Check_user(username) == 1)            /* Check if user is denied */
        puts("ERR");
    else
    {
    if (Valid_User(username, password, PRIMARY_DC, BACKUP_DC, NTDOMAIN) == 0)
       puts("OK");
    else
       puts("ERR");
  }

    fflush(stdout);
  }
  
  return 0;
}

/* Valid_User return codes -

   0 - User authenticated successfully.
   1 - Server error.
   2 - Protocol error.
   3 - Logon error; Incorrect password or username given.
*/

