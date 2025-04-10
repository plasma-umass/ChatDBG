/*==========================================================
 * config file utils for polymorph
 *==========================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>

#include "polymorph_types.h"
#include "rcfile.h"

/* xternal global vars */
extern polym_t *user_prefs;

/* filenames */
char init_file[] = "/polymorphrc\0";
char hidn_file[] = "/.polymorphrc\0";

/*----------------------------------------------------------
 * function:    grok_rcfile
 * parameters:  none
 * returns:     polym_t * to list of prefs
 * description: here's where we crack open a config file and
 * look up the specifics the user wants
 * note:        looks in $POLYMORPH_DIR, $HOME, /usr/local/etc,
 * and /etc in this order.  Searching stops when it finds
 * the first config file
 *----------------------------------------------------------*/
polym_t *grok_rcfile(){
  char where[MAX];
  int step, found;
  polym_t *list;
  FILE *rcfile;

  list = NULL;
  found = step = 0;
  
  /* find the config file and open *
   * a stream to it                */
  while( !found ){
    switch( step ){
    case 3: /* I hardly ever expect it here... */
      memset( where, '\0', MAX );
      strcpy( where, "/etc" );
      strcat( where, init_file );
      break;
    case 2: /* then this place */
      memset( where, '\0', MAX );
      strcpy( where, "/usr/local/etc" );
      strcat( where, init_file );
      break;
    case 1: /* $HOME/.polymorphrc next */
      memset( where, '\0', MAX );
      strcpy( where, getenv( "HOME" ) );
      strcat( where, hidn_file );
      break;
    case 0: /* look for $POLYMORPH_DIR first */
    default:
      memset( where, '\0', MAX );
      strcpy( where, getenv( "POLYMORPH_DIR" ) );
      strcat( where, init_file );
      break;
    }
    step++;        /* advance to the next step */
    rcfile = NULL; /* init the stream pointer */
    rcfile = fopen( where, "r" );
    found = 1;     /* be optimistic */
    if( rcfile == NULL ){
      fclose( rcfile );
      found = 0;   /* until you're proven wrong */
    }
    /* if you add other places to look, be sure to *
     * change the final step's index here...       */
    if( step >= 3 ) break; 
  }
  
  /* when we don't find one *
   * tell the program       */
  if( rcfile == NULL ) return( NULL );
  
  /* process the file for settings */
  list = parse_rcfile( rcfile, list );

  /* close the stream */
  fclose( rcfile );

  if( list == NULL ) fprintf( stderr, "list is NULL\n" );

  return( list );

}/* end of grok_rcfile */
/*----------------------------------------------------------
 * function:    parse_rcfile
 * parameters:  rcfile -- FILE * of the opened config file
 *              u_prefs -- polym_t * of user's requests for behavior
 * returns:     polym_t * head of list
 * description: this function actually reads the data out of
 * the file, the others are for organizing the search and 
 * prioritizing the order of preferences.
 *----------------------------------------------------------*/
polym_t *parse_rcfile(FILE *rcfile, polym_t *u_prefs){
  char target_dir[MAX], after_dir[MAX];
  char newxtn[XTN], buf[MAX], *colon;
  polym_t *tmpdir, *head;
  xtn_t *tmpxtn;
  int d, e, i;

  head = NULL;

  while( fgets( buf, sizeof( buf ), rcfile ) != NULL ){
    /* skip lines beginning with silly characters */
    if( buf[0] == '#' ) continue;
    if( buf[0] == '!' ) continue;
    if( buf[0] == ';' ) continue;
    if( buf[0] == ':' ) continue;

    /* skip lines not flush left */
    if( isspace( buf[0] ) ) continue;

    /* skip lines shorter than XTN */
    if( strlen(buf) < XTN ) continue;

    /* initialize data stuff */
    colon = strchr( buf, ':' );
    strcpy( target_dir, buf );
    strcpy( after_dir, colon );

    /* get directory entry */
    colon = strchr( target_dir, ':' );
    *colon = '\0';
    trim_whitespace( target_dir );
    
    /* add the element to the list */
    tmpdir = NULL;
    if( u_prefs == NULL ){                       /* when there's no list */
      tmpdir = add_dir( u_prefs, target_dir );   /* init one */
    }else{                                       /* otherwise */
      tmpdir = find_dir( u_prefs, target_dir );  /* look for dir in list*/
      if( tmpdir == NULL ){                      /* when it's not there */
        tmpdir = add_dir( u_prefs, target_dir ); /* add it */
      }
    }

    /* get all extension info */
    tmpxtn = NULL;
    e = 0;
    
    for(i=0;i<strlen(after_dir);i++){
      /* looking for spaces or commas between entries */
      if( isspace( after_dir[i] ) || after_dir[i] == ',' ){
        if( strlen( newxtn ) > 1 ){ /* handle */
          /* now, check to see if this extention is
           * already in the list we're tracking */
          if( find_xtn( tmpxtn, newxtn ) != NULL ){
            strcpy( newxtn, "" );
            continue;
          }
          /* it's new and real, so keep it */
          add_xtn( tmpxtn, newxtn );
          memset( newxtn, '\0', XTN );
          e = 0;
        }
        continue;
      }
      /* only copy the good stuff */
      if( isprint( after_dir[i] ) ) newxtn[e++] = after_dir[i];
    }
    tmpdir->extns = tmpxtn;
  }

  return( head );

}/* end of parse_rcfile */
/*==========================================================
 * function: trim_whitespace
 * parameters: victim -- char * to trim
 * notes: passes the cleaned string back through the
 * parameter pointer...
 *==========================================================*/
void trim_whitespace(char *victim){
  int i, j;
  char new_str[MAX];

  j = 0;
  
  for(i=0;i<strlen(victim);i++){
    if( isspace( victim[i] ) ) continue;
    new_str[j++] = victim[i];
  }

  new_str[j] = '\0';
  strcpy( victim, new_str );
  
}/* end of trim_whitespace */
/*==========================================================
 * end of rcfile.c
 *==========================================================*/
