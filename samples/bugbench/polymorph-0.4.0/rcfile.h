/* rcfile.h */

polym_t *grok_rcfile();
polym_t *parse_rcfile(FILE *rcfile, polym_t *u_prefs);
void trim_whitespace(char *victim);

extern polym_t *add_dir(polym_t *list, char *victim);
extern polym_t *find_dir(polym_t *list, char *suspect);
extern xtn_t *find_xtn(xtn_t *list, char *suspect);
extern xtn_t *add_xtn(xtn_t *list, char *victim);

