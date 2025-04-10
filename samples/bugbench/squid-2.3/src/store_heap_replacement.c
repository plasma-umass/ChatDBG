
/*
 * $Id: store_heap_replacement.c,v 1.1.2.8 2000/02/09 23:30:02 wessels Exp $
 *
 * DEBUG: section 20    Storage Manager Heap-based replacement
 * AUTHOR: John Dilley
 *
 * SQUID Internet Object Cache  http://squid.nlanr.net/Squid/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from the
 *  Internet community.  Development is led by Duane Wessels of the
 *  National Laboratory for Applied Network Research and funded by the
 *  National Science Foundation.  Squid is Copyrighted (C) 1998 by
 *  the Regents of the University of California.  Please see the
 *  COPYRIGHT file for full details.  Squid incorporates software
 *  developed and/or copyrighted by other sources.  Please see the
 *  CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

/*
 * The code in this file is Copyrighted (C) 1999 by Hewlett Packard.
 * 
 *
 * For a description of these cache replacement policies see --
 *  http://www.hpl.hp.com/techreports/1999/HPL-1999-69.html
 */

/*
 * Key generation function to implement the LFU-DA policy (Least
 * Frequently Used with Dynamic Aging).  Similar to classical LFU
 * but with aging to handle turnover of the popular document set.
 * Maximizes byte hit rate by keeping more currently popular objects
 * in cache regardless of size.  Achieves lower hit rate than GDS
 * because there are more large objects in cache (so less room for
 * smaller popular objects).
 * 
 * This version implements a tie-breaker based upon recency
 * (e->lastref): for objects that have the same reference count
 * the most recent object wins (gets a higher key value).
 */
static heap_key
HeapKeyGen_StoreEntry_LFUDA(void *entry, double age)
{
    StoreEntry *e = entry;
    double tie;
    if (e->lastref <= 0)
	tie = 0.0;
    else if (squid_curtime <= e->lastref)
	tie = 0.0;
    else
	tie = 1.0 - exp((double) (e->lastref - squid_curtime) / 86400.0);
    return age + (double) e->refcount - tie;
}


/*
 * Key generation function to implement the GDS-Frequency policy.
 * Similar to Greedy Dual-Size Hits policy, but adds aging of
 * documents to prevent pollution.  Maximizes object hit rate by
 * keeping more small, popular objects in cache.  Achieves lower
 * byte hit rate than LFUDA because there are fewer large objects
 * in cache.
 * 
 * This version implements a tie-breaker based upon recency
 * (e->lastref): for objects that have the same reference count
 * the most recent object wins (gets a higher key value).
 */
static heap_key
HeapKeyGen_StoreEntry_GDSF(void *entry, double age)
{
    StoreEntry *e = entry;
    double size = e->swap_file_sz ? (double) e->swap_file_sz : 1.0;
    double tie = (e->lastref > 1) ? (1.0 / e->lastref) : 1.0;
    return age + ((double) e->refcount / size) - tie;
}

/* 
 * Key generation function to implement the LRU policy.  Normally
 * one would not do this with a heap -- use the linked list instead.
 * For testing and performance characterization it was useful.
 * Don't use it unless you are trying to compare performance among
 * heap-based replacement policies...
 */
static heap_key
HeapKeyGen_StoreEntry_LRU(void *entry, double age)
{
    StoreEntry *e = entry;
    return (heap_key) e->lastref;
}
