/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SDBM_PAIR_H
#define SDBM_PAIR_H

/* Mini EMBED (pair.c) */
#define chkpage sdbm__chkpage
#define delpair sdbm__delpair
#define duppair sdbm__duppair
#define fitpair sdbm__fitpair
#define getnkey sdbm__getnkey
#define getpair sdbm__getpair
#define putpair sdbm__putpair
#define splpage sdbm__splpage

int fitpair(char *, int);
void  putpair(char *, apr_sdbm_datum_t, apr_sdbm_datum_t);
apr_sdbm_datum_t getpair(char *, apr_sdbm_datum_t);
int  delpair(char *, apr_sdbm_datum_t);
int  chkpage (char *);
apr_sdbm_datum_t getnkey(char *, int);
void splpage(char *, char *, long);
int duppair(char *, apr_sdbm_datum_t);

#endif /* SDBM_PAIR_H */

