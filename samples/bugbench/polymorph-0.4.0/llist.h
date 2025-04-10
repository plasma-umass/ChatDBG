/* llist.h */

xtn_t *find_xtn(xtn_t *list, char *suspect);
polym_t *find_dir(polym_t *list, char *suspect);
polym_t *add_dir(polym_t *list, char *victim);
xtn_t *add_xtn(xtn_t *list, char *victim);
xtn_t *find_last_xtn(xtn_t *target);
polym_t *find_last_dir(polym_t *target);


