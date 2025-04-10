/*========================================================
 * polymorph -- a Win32 -> Unix filename convertor
 * Copyright 1999 - 2001 by Gerall Kahla
 * <kahlage@users.sourceforge.net>
 *========================================================
 * boilerplate here
 *========================================================
 * please see the end of this file for revision history
 *========================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "polymorph_types.h"
#include "polymorph.h"

/* global variables & defines */
char tmpbuf[MAX], target[MAX], wd[MAX];
struct dirent *victim;
struct stat status;
DIR *curr_dir;
int	hidden = 0;	/* by default, do NOT convert hidden files */
int	track = 0;	/* by default, do NOT track symlinks */
int	clean = 0;	/* by default, do NOT clean backslashed names */

/*----------------------------------------------------------
 * function:		main
 * parameters:	standard
 * returns:			0 on success, 1 on failure
 * description:	where the fun begins...
 *----------------------------------------------------------*/
int main(int argc, char *argv[]){
  char filename[MAX];
  
  strcpy( target, "" );

  grok_commandLine( argc, argv );

  if( strlen(target) != 0 ){
    convert_fileName( target );
    return( 0 );
  }

  /*   move_toNewDir( target ); */

  strcpy( wd, "" );
  strcpy( filename, "" );

  getcwd( wd, sizeof( wd ) );

  curr_dir = opendir( wd );
  if( curr_dir == NULL ){
    fprintf( stderr, "polymorph could not open the current working directory\n" );
    fprintf( stderr, "maybe you don't have permissions?\n" );
    fprintf( stderr, "polymorph terminated\n" );
    exit( 1 );
  }

  while( ( victim = readdir( curr_dir ) ) != NULL ){
    /* check to see if victim is a regular file */
    if( track ){
      /* work on the actual file */
      if( stat( victim->d_name, &status ) == -1 ){
        fprintf( stderr,"polymorph encountered something funky\n" );
        fprintf( stderr,"polymorph terminated\n" );
        return( 2 );
      }
      if( S_ISREG( status.st_mode ) ){
        strcpy( filename, victim->d_name );
        convert_fileName( filename );
        /* move_toNewDir( filename ); */
      }
    }else{
      /* work on the symlink to the file */
      if( lstat( victim->d_name, &status ) == -1 ){
        fprintf( stderr,"polymorph encountered something funky\n" );
        fprintf( stderr,"polymorph terminated\n" );
        return( 2 );
      }
      if( S_ISREG( status.st_mode ) ){
        strcpy( filename, victim->d_name );
        convert_fileName( filename );
        /* move_toNewDir( filename ); */
      }
    }
  }

  closedir( curr_dir );

  return( 0 );

}/* end of main */
/*----------------------------------------------------------
 * function:		grok_commandLine
 * parameters:	argc -- int of the number of command line args
 * 							argv -- char pointer to an array of command line args
 * returns:			void
 * desciption:	set some variables for the program based on 
 *							user requests
 *----------------------------------------------------------*/
void grok_commandLine(int argc, char *argv[]){
	int o;

	while( ( o = getopt( argc, argv, "achtvf:" ) ) != -1 ){
		switch( o ){
			case 'a':
				hidden = 1;
				break;
			case 'c':
				clean = 1;
				break;
			case 'f':
				strcpy( target, optarg );
				break;
			case 'h':
				fprintf( stderr,"polymorph v%s -- filename convertor\n", VERSION );
        fprintf( stderr,"written by Gerall Kahla.\n\n" );
				fprintf( stderr,"-a  all      convert hidden files\n" );
				fprintf( stderr,"-c	clean		 reduce a file's name to just after the last backslash\n" );
				fprintf( stderr,"-f  file     convert this file to a name with all lowercase letters\n" );
				fprintf( stderr,"-h  help     print this message and exit\n" );
				fprintf( stderr,"-t  track    track down the targets of symlinks and convert them\n" );
				fprintf( stderr,"-v  version  print the version number and exit\n" );
				fprintf( stderr,"\n" );
				exit( 0 );
			case 't':
				track = 1;
				break;
			case 'v':
				fprintf( stderr,"polymorph v%s\n", VERSION );
				exit( 0 );
			default:
				fprintf( stderr,"please run 'polymorph -h' for commandline options\n" );
				fprintf( stderr,"polymorph terminated\n" );
				exit( 0 );
		}
	}

}/* end of grok_commandLine */
/*----------------------------------------------------------
 * function:		move_toNewDir
 * parameters:	victim -- char * of the newly converted file
 * returns:			void
 * description:	here's where we look up where the user wants
 *							the file to go after conversion, and move it
 *----------------------------------------------------------*/
/* void move_toNewDir(char *victim){
	polymm_t *tmpd;
	xtn_t *tmpx;

    if( user_prefs == NULL ) printf("user_prefs is NULL\n");

    tmpd = user_prefs;
    while( tmpd != NULL ){
        printf( "\ntarget_dir: %s @ %0lx\n", tmpd->dest, tmpd );
        tmpx = tmpd->extns;
        while( tmpx != NULL ){
            printf( ": %s\t", tmpx->x3 );
            (struct xtn_t *)tmpx = tmpx->next;
        }
        (struct polym_t *)tmpd = tmpd->next;
    }

	printf( "\n" );

}*/ /* end of move_toNewDir */
/*----------------------------------------------------------
 * function:		convert_fileName
 * parameters:	original -- char pointer to string of file's name
 * returns:			void
 * description:	here's where the grunt work begins.  convert,
 * check for newname in cwd, then move
 *----------------------------------------------------------*/
void convert_fileName(char *original){
	char newname[MAX];
	char *bslash;
	int i, error;
  
	error = 0;
	strcpy( newname, "" );

	if( is_fileHidden( original ) && !hidden ) return;

  if( does_nameHaveUppers( original ) ){
		/* convert the filename */
		for(i=0;i<strlen(original);i++){
			if( isupper( original[i] ) ){
				newname[i] = tolower( original[i] );
				continue;
			}
			newname[i] = original[i];
		}
		newname[i] = '\0';
  }else{
    strcpy( newname, original );
    error = -1;
  }

	/* check to see if we need to clean the name
   * of backslash Windows-path cruft */
  if( clean ){
    bslash = NULL;
    bslash = strrchr( newname, '\\' );
    if( bslash != NULL ) strcpy( newname, &bslash[1] );
  }
  
  if( error != -1 ){
    error = does_newnameExist( newname );
    if( error ){
      fprintf( stderr,"target exists -- skipping %s...\n", original );
      return;
    }
  }
  
	error = rename( original, newname );
	if( error ){
		fprintf( stderr,
             "polymorph had trouble converting %s to %s...\n",
             original,
             newname );
		fprintf( stderr,"the file is now possibly corrupt\n" );
	}

	strcpy( original, newname );

}/* end of convert_fileName */
/*----------------------------------------------------------
 * function:		does_newnameExist
 * parameters:	suspect -- char pointer of name to check
 * returns:			0 if the name does not exist, 1 if it does
 * description:	just being paranoid, don't want to whack files
 *----------------------------------------------------------*/
int does_newnameExist(char *suspect){
	struct dirent *looking;
  int found;
	DIR *tmp;

	strcpy( wd, "" );
	getcwd( wd, sizeof(wd) );

	tmp = opendir( wd );
	
	if( tmp == NULL ){
		fprintf( stderr,"polymorph could not open the current working directory\n" );
		fprintf( stderr,"maybe you don't have permissions?\n" );
		fprintf( stderr,"polymorph terminated\n" );
		exit( 1 );
	}

  found = 0;
  
	while( ( looking = readdir(tmp) ) != NULL ){
		if( !strcmp( suspect, looking->d_name ) ) found = 1;
	}

	closedir( tmp );
	return( found );

}/* end of does_newnameExist */
/*----------------------------------------------------------
 * function:		does_nameHaveUppers
 * parameters:	suspect -- char pointer of name to check
 * returns:			0 if the name has no uppercase letters in it
 *							1 otherwise
 * description:	now making the imp check before converting...
 *----------------------------------------------------------*/
int does_nameHaveUppers(char *suspect){
	int i;

	for(i=0;i<strlen(suspect);i++){
		if( isupper( suspect[i] ) ) return(1);
	}

	return( 0 );

}/* end of does_nameHaveUppers */
/*----------------------------------------------------------
 * function:		is_fileHidden
 * parameters:	suspect -- char pointer of name in question
 * returns:			0 if the file is not hidden, 1 if it is
 * description:	nice little boolean check for hidden-ness
 *----------------------------------------------------------*/
int is_fileHidden(char *suspect){

	if( suspect[0] == '.' ) return( 1 );

	return( 0 );

}/* end of is_fileHidden */
/*========================================================
 * 04/13/99 -- initial incarnation in 'golem' code
 * 01/01/00 -- added -c switch for cleaning filenames
 * 06/23/00 -- edited for bugs
 * 01/07/01 -- changed name to 'polymorph'
 *========================================================
 * end of polymorph.c
 *========================================================*/
