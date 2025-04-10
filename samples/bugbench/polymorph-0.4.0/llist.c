/*========================================================*
 * linked list implementation and utils for polymorph     *
 *========================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "polymorph_types.h"
#include "llist.h"

/*----------------------------------------------------------
 * function: find_xtn
 * parameters: list -- xtn_t * of the current extension list
 * suspect -- char * of the suspicious string
 * returns: xtn_t * of the node in question, NULL otherwise
 * description: looking to see if we've already done this
 * filetype in this destination directory
 *----------------------------------------------------------*/
xtn_t *find_xtn(xtn_t *list, char *suspect){
  xtn_t *tmp;

  for(tmp=list;tmp!=NULL;tmp=tmp->next){
    if( !strcmp( tmp->x3, suspect ) ) break;
  }

  return( tmp );

}/* end of find_xtn */
/*----------------------------------------------------------
 * function:		find_dir
 * parameters:	list -- polym_t * of the entire list
 *							suspect -- char * of the suspected clone
 * returns:			polym_t * to the first target dir entry
 *							NULL if suspect isn't found in list
 * description:	just making sure we don't repeat things...
 *----------------------------------------------------------*/
polym_t *find_dir(polym_t *list, char *suspect){
	polym_t *tmp;

	for(tmp=list;tmp!=NULL;tmp=tmp->next){
		if( !strcmp( tmp->dest, suspect ) ) break;
	}

	return( tmp );

}/* end of find_dir */
/*----------------------------------------------------------
 * function:		add_dir
 * parameters:	list -- polym_t * of the list to add to
 * 							victim -- char * of the dir to add to list
 * returns:			polym_t * of the list element created here
 * description:	just like the name implies, it adds a node to
 * the running list of target dirs and returns.
 * note:				malloc in use!
 *---------------------------------------------------------*/
polym_t *add_dir(polym_t *list, char *victim){
	polym_t *tmp, *last;

	/* create the new list element */
	tmp = malloc( sizeof ( polym_t ) );
	if( tmp == NULL ){
		fprintf(stderr,"polymorph ran out of memory while initializing\n");
		fprintf(stderr,"in the interest of saving files,\n");
		fprintf(stderr,"polymorph terminated\n");
		exit( 3 );
	}

	/* init new list element */
	strcpy( tmp->dest, victim );
	tmp->extns = NULL;
	tmp->next = NULL;

	/* insert it at the end of the list */
  last == NULL;
  last = find_last_dir( list );

  if( last == NULL ){
    list = tmp;
  }else{
    last->next = tmp;
  }
  
  return( tmp );

}/* end of add_dir */
/*----------------------------------------------------------
 * function:		add_xtn
 * parameters:	list -- xtn_t * of the working extension list
 * 							victim -- char * of the extension to add
 * returns:			xtn_t * of the newest element
 * description:	making adds to the extension list cool and easy
 *----------------------------------------------------------*/
xtn_t *add_xtn(xtn_t *list, char *victim){
	xtn_t *tmp, *last;

	tmp = malloc( sizeof( xtn_t ) );
	if( tmp == NULL ){
		fprintf(stderr,"polymorph ran out of memory while initializing\n");
		fprintf(stderr,"in the interest of saving files,\n");
		fprintf(stderr,"polymorph terminated\n");
		exit( 3 );
	}

	strcpy( tmp->x3, victim );
	tmp->next = NULL;

  last = NULL;
  last = find_last_xtn( list );

  if( last == NULL ){
    list = tmp;
  }else{
    last->next = tmp;
  }
  
  return( tmp );

}/* end of add_xtn */
/*------------------------------------------------------
 * function: find_last_xtn
 * parameters: target -- xtn_t pointer to list of extensions
 * returns: xtn_t * to the last element in the list
 * note: non-destructive!
 * returns: pointer to last element, or NULL on empty list
 *------------------------------------------------------*/
xtn_t *find_last_xtn(xtn_t *target){
  xtn_t *run;
  
  if( target == NULL ) return( NULL );

  for(run=target;run!=NULL;run=run->next){
    if( run->next == NULL ) return( (xtn_t *)run->next );
  }

  return( NULL );

}/* end of find_last_xtn */
/*------------------------------------------------------
 * function: find_last_dir
 * parameters: target -- polym_t pointer to list
 * returns: polym_t pointer to last element in list, or
 * NULL on an empty list
 * note: non-destructive!
 *------------------------------------------------------*/
polym_t *find_last_dir(polym_t *target){
  polym_t *run;

  if( target == NULL ) return( NULL );

  for(run=target;run!=NULL;run=run->next){
    if( run->next == NULL ) return( (polym_t *)run->next );
  }
  
  return( NULL );
  
}/* end of find_last_dir */
/*======================================================
 * end of llist.c
 *======================================================*/
