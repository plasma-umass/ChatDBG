/*===============================
 * polymorph_types.h
 *===============================*/
#ifndef GK_GTYPES_H
#define GK_GTYPES_H

#include <stddef.h>

#define MAX	2048
#define XTN	16

typedef struct{
  char x3[XTN];
  struct xtn_t *next;
} xtn_t;

typedef struct{
  char dest[MAX];
  xtn_t *extns;
  struct polym_t *next;
} polym_t;

#endif

/* end of polymorph_types.h */
