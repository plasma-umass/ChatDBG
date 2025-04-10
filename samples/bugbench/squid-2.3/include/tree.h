/* tree.h - declare structures used by tree library
 *
 * vix 22jan93 [revisited; uses RCS, ANSI, POSIX; has bug fixes]
 * vix 27jun86 [broken out of tree.c]
 *
 * $Id: tree.h,v 1.4 1997/11/03 20:05:40 wessels Exp $
 */


#ifndef	_TREE_H_INCLUDED
#define	_TREE_H_INCLUDED

typedef struct tree_s {
    void *data;
    struct tree_s *left, *right;
    int bal;
} tree;

typedef int BTREE_CMP(void *, void *);
typedef int BTREE_UAR(void *);

void tree_init(tree **);
void *tree_srch(tree **, BTREE_CMP *, void *);
void *tree_add(tree **, int (*)(), void *, BTREE_UAR *);
int tree_delete(tree **, BTREE_CMP *, void *, BTREE_UAR *);
int tree_trav(tree **, BTREE_UAR *);
void tree_mung(tree **, BTREE_UAR *);

#endif /* _TREE_H_INCLUDED */
