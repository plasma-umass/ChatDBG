
/*
 * $Id: store_dir_ufs.c,v 1.17.2.8 2000/02/09 23:30:02 wessels Exp $
 *
 * DEBUG: section 47    Store Directory Routines
 * AUTHOR: Duane Wessels
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

#include "squid.h"
#if HAVE_STATVFS
#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#endif

#define DefaultLevelOneDirs     16
#define DefaultLevelTwoDirs     256
#define STORE_META_BUFSZ 4096

typedef struct _RebuildState RebuildState;
struct _RebuildState {
    SwapDir *sd;
    int n_read;
    FILE *log;
    int speed;
    int curlvl1;
    int curlvl2;
    struct {
	unsigned int need_to_validate:1;
	unsigned int clean:1;
	unsigned int init:1;
    } flags;
    int done;
    int in_dir;
    int fn;
    struct dirent *entry;
    DIR *td;
    char fullpath[SQUID_MAXPATHLEN];
    char fullfilename[SQUID_MAXPATHLEN];
    struct _store_rebuild_data counts;
};

static int n_ufs_dirs = 0;
static int *ufs_dir_index = NULL;

static char *storeUfsSwapSubDir(SwapDir *, int subdirn);
static int storeUfsCreateDirectory(const char *path, int);
static int storeUfsVerifyCacheDirs(SwapDir *);
static int storeUfsVerifyDirectory(const char *path);
static void storeUfsCreateSwapSubDirs(SwapDir *);
static char *storeUfsDirSwapLogFile(SwapDir *, const char *);
static EVH storeRebuildFromDirectory;
static EVH storeRebuildFromSwapLog;
static int storeGetNextFile(RebuildState *, int *sfileno, int *size);
static StoreEntry *storeAddDiskRestore(const cache_key * key,
    int file_number,
    size_t swap_file_sz,
    time_t expires,
    time_t timestamp,
    time_t lastref,
    time_t lastmod,
    u_num32 refcount,
    u_short flags,
    int clean);
static void storeUfsDirRebuild(SwapDir * sd);
static void storeUfsDirCloseTmpSwapLog(SwapDir * sd);
static FILE *storeUfsDirOpenTmpSwapLog(SwapDir *, int *, int *);
static STLOGOPEN storeUfsDirOpenSwapLog;
static STINIT storeUfsDirInit;
static STLOGCLEANOPEN storeUfsDirWriteCleanOpen;
static void storeUfsDirWriteCleanClose(SwapDir * sd);
static STLOGCLEANWRITE storeUfsDirWriteCleanEntry;
static STLOGCLOSE storeUfsDirCloseSwapLog;
static STLOGWRITE storeUfsDirSwapLog;
static STNEWFS storeUfsDirNewfs;
static QS rev_int_sort;
static int storeUfsDirClean(int swap_index);
static EVH storeUfsDirCleanEvent;
static int storeUfsDirIs(SwapDir * sd);
static int storeUfsFilenoBelongsHere(int fn, int F0, int F1, int F2);

static char *
storeUfsSwapSubDir(SwapDir * sd, int subdirn)
{
    LOCAL_ARRAY(char, fullfilename, SQUID_MAXPATHLEN);
    assert(0 <= subdirn && subdirn < sd->u.ufs.l1);
    snprintf(fullfilename, SQUID_MAXPATHLEN, "%s/%02X", sd->path, subdirn);
    return fullfilename;
}

static int
storeUfsCreateDirectory(const char *path, int should_exist)
{
    int created = 0;
    struct stat st;
    getCurrentTime();
    if (0 == stat(path, &st)) {
	if (S_ISDIR(st.st_mode)) {
	    debug(20, should_exist ? 3 : 1) ("%s exists\n", path);
	} else {
	    fatalf("Swap directory %s is not a directory.", path);
	}
    } else if (0 == mkdir(path, 0755)) {
	debug(20, should_exist ? 1 : 3) ("%s created\n", path);
	created = 1;
    } else {
	fatalf("Failed to make swap directory %s: %s",
	    path, xstrerror());
    }
    return created;
}

static int
storeUfsVerifyDirectory(const char *path)
{
    struct stat sb;
    if (stat(path, &sb) < 0) {
	debug(20, 0) ("%s: %s\n", path, xstrerror());
	return -1;
    }
    if (S_ISDIR(sb.st_mode) == 0) {
	debug(20, 0) ("%s is not a directory\n", path);
	return -1;
    }
    return 0;
}

/*
 * This function is called by storeUfsDirInit().  If this returns < 0,
 * then Squid exits, complains about swap directories not
 * existing, and instructs the admin to run 'squid -z'
 */
static int
storeUfsVerifyCacheDirs(SwapDir * sd)
{
    int j;
    const char *path = sd->path;
    if (storeUfsVerifyDirectory(path) < 0)
	return -1;
    for (j = 0; j < sd->u.ufs.l1; j++) {
	path = storeUfsSwapSubDir(sd, j);
	if (storeUfsVerifyDirectory(path) < 0)
	    return -1;
    }
    return 0;
}

static void
storeUfsCreateSwapSubDirs(SwapDir * sd)
{
    int i, k;
    int should_exist;
    LOCAL_ARRAY(char, name, MAXPATHLEN);
    for (i = 0; i < sd->u.ufs.l1; i++) {
	snprintf(name, MAXPATHLEN, "%s/%02X", sd->path, i);
	if (storeUfsCreateDirectory(name, 0))
	    should_exist = 0;
	else
	    should_exist = 1;
	debug(47, 1) ("Making directories in %s\n", name);
	for (k = 0; k < sd->u.ufs.l2; k++) {
	    snprintf(name, MAXPATHLEN, "%s/%02X/%02X", sd->path, i, k);
	    storeUfsCreateDirectory(name, should_exist);
	}
    }
}

static char *
storeUfsDirSwapLogFile(SwapDir * sd, const char *ext)
{
    LOCAL_ARRAY(char, path, SQUID_MAXPATHLEN);
    LOCAL_ARRAY(char, digit, 32);
    if (Config.Log.swap) {
	xstrncpy(path, Config.Log.swap, SQUID_MAXPATHLEN - 64);
	strcat(path, ".");
	snprintf(digit, 32, "%02d", sd->index);
	strncat(path, digit, 3);
    } else {
	xstrncpy(path, sd->path, SQUID_MAXPATHLEN - 64);
	strcat(path, "/swap.state");
    }
    if (ext)
	strncat(path, ext, 16);
    return path;
}

static void
storeUfsDirOpenSwapLog(SwapDir * sd)
{
    char *path;
    int fd;
    path = storeUfsDirSwapLogFile(sd, NULL);
    fd = file_open(path, O_WRONLY | O_CREAT);
    if (fd < 0) {
	debug(50, 1) ("%s: %s\n", path, xstrerror());
	fatal("storeUfsDirOpenSwapLog: Failed to open swap log.");
    }
    debug(47, 3) ("Cache Dir #%d log opened on FD %d\n", sd->index, fd);
    sd->u.ufs.swaplog_fd = fd;
    if (0 == n_ufs_dirs)
	assert(NULL == ufs_dir_index);
    n_ufs_dirs++;
    assert(n_ufs_dirs <= Config.cacheSwap.n_configured);
}

static void
storeUfsDirCloseSwapLog(SwapDir * sd)
{
    if (sd->u.ufs.swaplog_fd < 0)	/* not open */
	return;
    file_close(sd->u.ufs.swaplog_fd);
    debug(47, 3) ("Cache Dir #%d log closed on FD %d\n",
	sd->index, sd->u.ufs.swaplog_fd);
    sd->u.ufs.swaplog_fd = -1;
    n_ufs_dirs--;
    assert(n_ufs_dirs >= 0);
    if (0 == n_ufs_dirs)
	safe_free(ufs_dir_index);
}

static void
storeUfsDirInit(SwapDir * sd)
{
    static int started_clean_event = 0;
    static const char *errmsg =
    "\tFailed to verify one of the swap directories, Check cache.log\n"
    "\tfor details.  Run 'squid -z' to create swap directories\n"
    "\tif needed, or if running Squid for the first time.";
    if (storeUfsVerifyCacheDirs(sd) < 0)
	fatal(errmsg);
    storeUfsDirOpenSwapLog(sd);
    storeUfsDirRebuild(sd);
    if (!started_clean_event) {
	eventAdd("storeDirClean", storeUfsDirCleanEvent, NULL, 15.0, 1);
	started_clean_event = 1;
    }
}

static void
storeRebuildFromDirectory(void *data)
{
    RebuildState *rb = data;
    LOCAL_ARRAY(char, hdr_buf, DISK_PAGE_SIZE);
    StoreEntry *e = NULL;
    StoreEntry tmpe;
    cache_key key[MD5_DIGEST_CHARS];
    int sfileno = 0;
    int count;
    int size;
    struct stat sb;
    int swap_hdr_len;
    int fd = -1;
    tlv *tlv_list;
    tlv *t;
    assert(rb != NULL);
    if (opt_foreground_rebuild)
	getCurrentTime();
    debug(20, 3) ("storeRebuildFromDirectory: DIR #%d\n", rb->sd->index);
    for (count = 0; count < rb->speed; count++) {
	assert(fd == -1);
	fd = storeGetNextFile(rb, &sfileno, &size);
	if (fd == -2) {
	    debug(20, 1) ("Done scanning %s swaplog (%d entries)\n",
		rb->sd->path, rb->n_read);
	    store_dirs_rebuilding--;
	    storeUfsDirCloseTmpSwapLog(rb->sd);
	    storeRebuildComplete(&rb->counts);
	    cbdataFree(rb);
	    return;
	} else if (fd < 0) {
	    continue;
	}
	assert(fd > -1);
	/* lets get file stats here */
	if (fstat(fd, &sb) < 0) {
	    debug(20, 1) ("storeRebuildFromDirectory: fstat(FD %d): %s\n",
		fd, xstrerror());
	    file_close(fd);
	    store_open_disk_fd--;
	    fd = -1;
	    continue;
	}
	if ((++rb->counts.scancount & 0xFFFF) == 0)
	    debug(20, 3) ("  %s %7d files opened so far.\n",
		rb->sd->path, rb->counts.scancount);
	debug(20, 9) ("file_in: fd=%d %08X\n", fd, sfileno);
	Counter.syscalls.disk.reads++;
	if (read(fd, hdr_buf, DISK_PAGE_SIZE) < 0) {
	    debug(20, 1) ("storeRebuildFromDirectory: read(FD %d): %s\n",
		fd, xstrerror());
	    file_close(fd);
	    store_open_disk_fd--;
	    fd = -1;
	    continue;
	}
	file_close(fd);
	store_open_disk_fd--;
	fd = -1;
	swap_hdr_len = 0;
#if USE_TRUNCATE
	if (sb.st_size == 0)
	    continue;
#endif
	tlv_list = storeSwapMetaUnpack(hdr_buf, &swap_hdr_len);
	if (tlv_list == NULL) {
	    debug(20, 1) ("storeRebuildFromDirectory: failed to get meta data\n");
	    storeUnlink(sfileno);
	    continue;
	}
	debug(20, 3) ("storeRebuildFromDirectory: successful swap meta unpacking\n");
	memset(key, '\0', MD5_DIGEST_CHARS);
	memset(&tmpe, '\0', sizeof(StoreEntry));
	for (t = tlv_list; t; t = t->next) {
	    switch (t->type) {
	    case STORE_META_KEY:
		assert(t->length == MD5_DIGEST_CHARS);
		xmemcpy(key, t->value, MD5_DIGEST_CHARS);
		break;
	    case STORE_META_STD:
		assert(t->length == STORE_HDR_METASIZE);
		xmemcpy(&tmpe.timestamp, t->value, STORE_HDR_METASIZE);
		break;
	    default:
		break;
	    }
	}
	storeSwapTLVFree(tlv_list);
	tlv_list = NULL;
	if (storeKeyNull(key)) {
	    debug(20, 1) ("storeRebuildFromDirectory: NULL key\n");
	    storeUnlink(sfileno);
	    continue;
	}
	tmpe.key = key;
	/* check sizes */
	if (tmpe.swap_file_sz == 0) {
	    tmpe.swap_file_sz = sb.st_size;
	} else if (tmpe.swap_file_sz == sb.st_size - swap_hdr_len) {
	    tmpe.swap_file_sz = sb.st_size;
	} else if (tmpe.swap_file_sz != sb.st_size) {
	    debug(20, 1) ("storeRebuildFromDirectory: SIZE MISMATCH %d!=%d\n",
		tmpe.swap_file_sz, (int) sb.st_size);
	    storeUnlink(sfileno);
	    continue;
	}
	if (EBIT_TEST(tmpe.flags, KEY_PRIVATE)) {
	    storeUnlink(sfileno);
	    rb->counts.badflags++;
	    continue;
	}
	e = storeGet(key);
	if (e && e->lastref >= tmpe.lastref) {
	    /* key already exists, current entry is newer */
	    /* keep old, ignore new */
	    rb->counts.dupcount++;
	    continue;
	} else if (NULL != e) {
	    /* URL already exists, this swapfile not being used */
	    /* junk old, load new */
	    storeRelease(e);	/* release old entry */
	    rb->counts.dupcount++;
	}
	rb->counts.objcount++;
	storeEntryDump(&tmpe, 5);
	e = storeAddDiskRestore(key,
	    sfileno,
	    tmpe.swap_file_sz,
	    tmpe.expires,
	    tmpe.timestamp,
	    tmpe.lastref,
	    tmpe.lastmod,
	    tmpe.refcount,	/* refcount */
	    tmpe.flags,		/* flags */
	    (int) rb->flags.clean);
    }
    eventAdd("storeRebuild", storeRebuildFromDirectory, rb, 0.0, 1);
}

static void
storeRebuildFromSwapLog(void *data)
{
    RebuildState *rb = data;
    StoreEntry *e = NULL;
    storeSwapLogData s;
    size_t ss = sizeof(storeSwapLogData);
    int count;
    int used;			/* is swapfile already in use? */
    int disk_entry_newer;	/* is the log entry newer than current entry? */
    double x;
    assert(rb != NULL);
    /* load a number of objects per invocation */
    for (count = 0; count < rb->speed; count++) {
	if (fread(&s, ss, 1, rb->log) != 1) {
	    debug(20, 1) ("Done reading %s swaplog (%d entries)\n",
		rb->sd->path, rb->n_read);
	    fclose(rb->log);
	    rb->log = NULL;
	    store_dirs_rebuilding--;
	    storeUfsDirCloseTmpSwapLog(rb->sd);
	    storeRebuildComplete(&rb->counts);
	    cbdataFree(rb);
	    return;
	}
	rb->n_read++;
	if (s.op <= SWAP_LOG_NOP)
	    continue;
	if (s.op >= SWAP_LOG_MAX)
	    continue;
	s.swap_file_number = storeDirProperFileno(rb->sd->index, s.swap_file_number);
	debug(20, 3) ("storeRebuildFromSwapLog: %s %s %08X\n",
	    swap_log_op_str[(int) s.op],
	    storeKeyText(s.key),
	    s.swap_file_number);
	if (s.op == SWAP_LOG_ADD) {
	    (void) 0;
	} else if (s.op == SWAP_LOG_DEL) {
	    if ((e = storeGet(s.key)) != NULL) {
		/*
		 * Make sure we don't unlink the file, it might be
		 * in use by a subsequent entry.  Also note that
		 * we don't have to subtract from store_swap_size
		 * because adding to store_swap_size happens in
		 * the cleanup procedure.
		 */
		storeExpireNow(e);
		storeReleaseRequest(e);
		if (e->swap_file_number > -1) {
		    storeDirMapBitReset(e->swap_file_number);
		    e->swap_file_number = -1;
		}
		rb->counts.objcount--;
		rb->counts.cancelcount++;
	    }
	    continue;
	} else {
	    x = log(++rb->counts.bad_log_op) / log(10.0);
	    if (0.0 == x - (double) (int) x)
		debug(20, 1) ("WARNING: %d invalid swap log entries found\n",
		    rb->counts.bad_log_op);
	    rb->counts.invalid++;
	    continue;
	}
	if ((++rb->counts.scancount & 0xFFFF) == 0)
	    debug(20, 3) ("  %7d %s Entries read so far.\n",
		rb->counts.scancount, rb->sd->path);
	if (!storeDirValidFileno(s.swap_file_number, 0)) {
	    rb->counts.invalid++;
	    continue;
	}
	if (EBIT_TEST(s.flags, KEY_PRIVATE)) {
	    rb->counts.badflags++;
	    continue;
	}
	e = storeGet(s.key);
	used = storeDirMapBitTest(s.swap_file_number);
	/* If this URL already exists in the cache, does the swap log
	 * appear to have a newer entry?  Compare 'lastref' from the
	 * swap log to e->lastref. */
	disk_entry_newer = e ? (s.lastref > e->lastref ? 1 : 0) : 0;
	if (used && !disk_entry_newer) {
	    /* log entry is old, ignore it */
	    rb->counts.clashcount++;
	    continue;
	} else if (used && e && e->swap_file_number == s.swap_file_number) {
	    /* swapfile taken, same URL, newer, update meta */
	    if (e->store_status == STORE_OK) {
		e->lastref = s.timestamp;
		e->timestamp = s.timestamp;
		e->expires = s.expires;
		e->lastmod = s.lastmod;
		e->flags = s.flags;
		e->refcount += s.refcount;
#if HEAP_REPLACEMENT
		storeHeapPositionUpdate(e);
#endif
	    } else {
		debug_trap("storeRebuildFromSwapLog: bad condition");
		debug(20, 1) ("\tSee %s:%d\n", __FILE__, __LINE__);
	    }
	    continue;
	} else if (used) {
	    /* swapfile in use, not by this URL, log entry is newer */
	    /* This is sorta bad: the log entry should NOT be newer at this
	     * point.  If the log is dirty, the filesize check should have
	     * caught this.  If the log is clean, there should never be a
	     * newer entry. */
	    debug(20, 1) ("WARNING: newer swaplog entry for fileno %08X\n",
		s.swap_file_number);
	    /* I'm tempted to remove the swapfile here just to be safe,
	     * but there is a bad race condition in the NOVM version if
	     * the swapfile has recently been opened for writing, but
	     * not yet opened for reading.  Because we can't map
	     * swapfiles back to StoreEntrys, we don't know the state
	     * of the entry using that file.  */
	    /* We'll assume the existing entry is valid, probably because
	     * were in a slow rebuild and the the swap file number got taken
	     * and the validation procedure hasn't run. */
	    assert(rb->flags.need_to_validate);
	    rb->counts.clashcount++;
	    continue;
	} else if (e && !disk_entry_newer) {
	    /* key already exists, current entry is newer */
	    /* keep old, ignore new */
	    rb->counts.dupcount++;
	    continue;
	} else if (e) {
	    /* key already exists, this swapfile not being used */
	    /* junk old, load new */
	    storeExpireNow(e);
	    storeReleaseRequest(e);
	    if (e->swap_file_number > -1) {
		storeDirMapBitReset(e->swap_file_number);
		e->swap_file_number = -1;
	    }
	    rb->counts.dupcount++;
	} else {
	    /* URL doesnt exist, swapfile not in use */
	    /* load new */
	    (void) 0;
	}
	/* update store_swap_size */
	rb->counts.objcount++;
	e = storeAddDiskRestore(s.key,
	    s.swap_file_number,
	    s.swap_file_sz,
	    s.expires,
	    s.timestamp,
	    s.lastref,
	    s.lastmod,
	    s.refcount,
	    s.flags,
	    (int) rb->flags.clean);
	storeDirSwapLog(e, SWAP_LOG_ADD);
    }
    eventAdd("storeRebuild", storeRebuildFromSwapLog, rb, 0.0, 1);
}

static int
storeGetNextFile(RebuildState * rb, int *sfileno, int *size)
{
    int fd = -1;
    int used = 0;
    int dirs_opened = 0;
    debug(20, 3) ("storeGetNextFile: flag=%d, %d: /%02X/%02X\n",
	rb->flags.init,
	rb->sd->index,
	rb->curlvl1,
	rb->curlvl2);
    if (rb->done)
	return -2;
    while (fd < 0 && rb->done == 0) {
	fd = -1;
	if (0 == rb->flags.init) {	/* initialize, open first file */
	    rb->done = 0;
	    rb->curlvl1 = 0;
	    rb->curlvl2 = 0;
	    rb->in_dir = 0;
	    rb->flags.init = 1;
	    assert(Config.cacheSwap.n_configured > 0);
	}
	if (0 == rb->in_dir) {	/* we need to read in a new directory */
	    snprintf(rb->fullpath, SQUID_MAXPATHLEN, "%s/%02X/%02X",
		rb->sd->path,
		rb->curlvl1, rb->curlvl2);
	    if (rb->flags.init && rb->td != NULL)
		closedir(rb->td);
	    rb->td = NULL;
	    if (dirs_opened)
		return -1;
	    rb->td = opendir(rb->fullpath);
	    dirs_opened++;
	    if (rb->td == NULL) {
		debug(50, 1) ("storeGetNextFile: opendir: %s: %s\n",
		    rb->fullpath, xstrerror());
	    } else {
		rb->entry = readdir(rb->td);	/* skip . and .. */
		rb->entry = readdir(rb->td);
		if (rb->entry == NULL && errno == ENOENT)
		    debug(20, 1) ("storeGetNextFile: directory does not exist!.\n");
		debug(20, 3) ("storeGetNextFile: Directory %s\n", rb->fullpath);
	    }
	}
	if (rb->td != NULL && (rb->entry = readdir(rb->td)) != NULL) {
	    rb->in_dir++;
	    if (sscanf(rb->entry->d_name, "%x", &rb->fn) != 1) {
		debug(20, 3) ("storeGetNextFile: invalid %s\n",
		    rb->entry->d_name);
		continue;
	    }
	    if (!storeUfsFilenoBelongsHere(rb->fn, rb->sd->index, rb->curlvl1, rb->curlvl2)) {
		debug(20, 3) ("storeGetNextFile: %08X does not belong in %d/%d/%d\n",
		    rb->fn, rb->sd->index, rb->curlvl1, rb->curlvl2);
		continue;
	    }
	    rb->fn = storeDirProperFileno(rb->sd->index, rb->fn);
	    used = storeDirMapBitTest(rb->fn);
	    if (used) {
		debug(20, 3) ("storeGetNextFile: Locked, continuing with next.\n");
		continue;
	    }
	    snprintf(rb->fullfilename, SQUID_MAXPATHLEN, "%s/%s",
		rb->fullpath, rb->entry->d_name);
	    debug(20, 3) ("storeGetNextFile: Opening %s\n", rb->fullfilename);
	    fd = file_open(rb->fullfilename, O_RDONLY);
	    if (fd < 0)
		debug(50, 1) ("storeGetNextFile: %s: %s\n", rb->fullfilename, xstrerror());
	    else
		store_open_disk_fd++;
	    continue;
	}
	rb->in_dir = 0;
	if (++rb->curlvl2 < rb->sd->u.ufs.l2)
	    continue;
	rb->curlvl2 = 0;
	if (++rb->curlvl1 < rb->sd->u.ufs.l1)
	    continue;
	rb->curlvl1 = 0;
	rb->done = 1;
    }
    *sfileno = rb->fn;
    return fd;
}

/* Add a new object to the cache with empty memory copy and pointer to disk
 * use to rebuild store from disk. */
static StoreEntry *
storeAddDiskRestore(const cache_key * key,
    int file_number,
    size_t swap_file_sz,
    time_t expires,
    time_t timestamp,
    time_t lastref,
    time_t lastmod,
    u_num32 refcount,
    u_short flags,
    int clean)
{
    StoreEntry *e = NULL;
    debug(20, 5) ("StoreAddDiskRestore: %s, fileno=%08X\n", storeKeyText(key), file_number);
    /* if you call this you'd better be sure file_number is not 
     * already in use! */
    e = new_StoreEntry(STORE_ENTRY_WITHOUT_MEMOBJ, NULL, NULL);
    e->store_status = STORE_OK;
    storeSetMemStatus(e, NOT_IN_MEMORY);
    e->swap_status = SWAPOUT_DONE;
    e->swap_file_number = file_number;
    e->swap_file_sz = swap_file_sz;
    e->lock_count = 0;
#if !HEAP_REPLACEMENT
    e->refcount = 0;
#endif
    e->lastref = lastref;
    e->timestamp = timestamp;
    e->expires = expires;
    e->lastmod = lastmod;
    e->refcount = refcount;
    e->flags = flags;
    EBIT_SET(e->flags, ENTRY_CACHABLE);
    EBIT_CLR(e->flags, RELEASE_REQUEST);
    EBIT_CLR(e->flags, KEY_PRIVATE);
    e->ping_status = PING_NONE;
    EBIT_CLR(e->flags, ENTRY_VALIDATED);
    storeDirMapBitSet(e->swap_file_number);
    storeHashInsert(e, key);	/* do it after we clear KEY_PRIVATE */
    return e;
}

static void
storeUfsDirRebuild(SwapDir * sd)
{
    RebuildState *rb = xcalloc(1, sizeof(*rb));
    int clean = 0;
    int zero = 0;
    FILE *fp;
    EVH *func = NULL;
    rb->sd = sd;
    rb->speed = opt_foreground_rebuild ? 1 << 30 : 50;
    /*
     * If the swap.state file exists in the cache_dir, then
     * we'll use storeRebuildFromSwapLog(), otherwise we'll
     * use storeRebuildFromDirectory() to open up each file
     * and suck in the meta data.
     */
    fp = storeUfsDirOpenTmpSwapLog(sd, &clean, &zero);
    if (fp == NULL || zero) {
	if (fp != NULL)
	    fclose(fp);
	func = storeRebuildFromDirectory;
    } else {
	func = storeRebuildFromSwapLog;
	rb->log = fp;
	rb->flags.clean = (unsigned int) clean;
    }
    if (!clean)
	rb->flags.need_to_validate = 1;
    debug(20, 1) ("Rebuilding storage in %s (%s)\n",
	sd->path, clean ? "CLEAN" : "DIRTY");
    store_dirs_rebuilding++;
    cbdataAdd(rb, cbdataXfree, 0);
    eventAdd("storeRebuild", func, rb, 0.0, 1);
}

static void
storeUfsDirCloseTmpSwapLog(SwapDir * sd)
{
    char *swaplog_path = xstrdup(storeUfsDirSwapLogFile(sd, NULL));
    char *new_path = xstrdup(storeUfsDirSwapLogFile(sd, ".new"));
    int fd;
    file_close(sd->u.ufs.swaplog_fd);
#ifdef _SQUID_OS2_
    if (unlink(swaplog_path) < 0) {
	debug(50, 0) ("%s: %s\n", swaplog_path, xstrerror());
	fatal("storeUfsDirCloseTmpSwapLog: unlink failed");
    }
#endif
    if (xrename(new_path, swaplog_path) < 0) {
	fatal("storeUfsDirCloseTmpSwapLog: rename failed");
    }
    fd = file_open(swaplog_path, O_WRONLY | O_CREAT);
    if (fd < 0) {
	debug(50, 1) ("%s: %s\n", swaplog_path, xstrerror());
	fatal("storeUfsDirCloseTmpSwapLog: Failed to open swap log.");
    }
    safe_free(swaplog_path);
    safe_free(new_path);
    sd->u.ufs.swaplog_fd = fd;
    debug(47, 3) ("Cache Dir #%d log opened on FD %d\n", sd->index, fd);
}

static FILE *
storeUfsDirOpenTmpSwapLog(SwapDir * sd, int *clean_flag, int *zero_flag)
{
    char *swaplog_path = xstrdup(storeUfsDirSwapLogFile(sd, NULL));
    char *clean_path = xstrdup(storeUfsDirSwapLogFile(sd, ".last-clean"));
    char *new_path = xstrdup(storeUfsDirSwapLogFile(sd, ".new"));
    struct stat log_sb;
    struct stat clean_sb;
    FILE *fp;
    int fd;
    if (stat(swaplog_path, &log_sb) < 0) {
	debug(47, 1) ("Cache Dir #%d: No log file\n", sd->index);
	safe_free(swaplog_path);
	safe_free(clean_path);
	safe_free(new_path);
	return NULL;
    }
    *zero_flag = log_sb.st_size == 0 ? 1 : 0;
    /* close the existing write-only FD */
    if (sd->u.ufs.swaplog_fd >= 0)
	file_close(sd->u.ufs.swaplog_fd);
    /* open a write-only FD for the new log */
    fd = file_open(new_path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
	debug(50, 1) ("%s: %s\n", new_path, xstrerror());
	fatal("storeDirOpenTmpSwapLog: Failed to open swap log.");
    }
    sd->u.ufs.swaplog_fd = fd;
    /* open a read-only stream of the old log */
    fp = fopen(swaplog_path, "r");
    if (fp == NULL) {
	debug(50, 0) ("%s: %s\n", swaplog_path, xstrerror());
	fatal("Failed to open swap log for reading");
    }
    memset(&clean_sb, '\0', sizeof(struct stat));
    if (stat(clean_path, &clean_sb) < 0)
	*clean_flag = 0;
    else if (clean_sb.st_mtime < log_sb.st_mtime)
	*clean_flag = 0;
    else
	*clean_flag = 1;
    safeunlink(clean_path, 1);
    safe_free(swaplog_path);
    safe_free(clean_path);
    safe_free(new_path);
    return fp;
}

struct _clean_state {
    char *cur;
    char *new;
    char *cln;
    char *outbuf;
    off_t outbuf_offset;
    int fd;
};

#define CLEAN_BUF_SZ 16384
/*
 * Begin the process to write clean cache state.  For UFS this means
 * opening some log files and allocating write buffers.  Return 0 if
 * we succeed, and assign the 'func' and 'data' return pointers.
 */
static int
storeUfsDirWriteCleanOpen(SwapDir * sd)
{
    struct _clean_state *state = xcalloc(1, sizeof(*state));
    struct stat sb;
    sd->log.clean.write = NULL;
    sd->log.clean.state = NULL;
    state->cur = xstrdup(storeUfsDirSwapLogFile(sd, NULL));
    state->new = xstrdup(storeUfsDirSwapLogFile(sd, ".clean"));
    state->cln = xstrdup(storeUfsDirSwapLogFile(sd, ".last-clean"));
    state->outbuf = xcalloc(CLEAN_BUF_SZ, 1);
    state->outbuf_offset = 0;
    unlink(state->new);
    unlink(state->cln);
    state->fd = file_open(state->new, O_WRONLY | O_CREAT | O_TRUNC);
    if (state->fd < 0)
	return -1;
    debug(20, 3) ("storeDirWriteCleanLogs: opened %s, FD %d\n",
	state->new, state->fd);
#if HAVE_FCHMOD
    if (stat(state->cur, &sb) == 0)
	fchmod(state->fd, sb.st_mode);
#endif
    sd->log.clean.write = storeUfsDirWriteCleanEntry;
    sd->log.clean.state = state;
    return 0;
}

/*
 * "write" an entry to the clean log file.
 */
static void
storeUfsDirWriteCleanEntry(const StoreEntry * e, SwapDir * sd)
{
    storeSwapLogData s;
    static size_t ss = sizeof(storeSwapLogData);
    struct _clean_state *state = sd->log.clean.state;
    if (NULL == e) {
	storeUfsDirWriteCleanClose(sd);
	return;
    }
    memset(&s, '\0', ss);
    s.op = (char) SWAP_LOG_ADD;
    s.swap_file_number = e->swap_file_number;
    s.timestamp = e->timestamp;
    s.lastref = e->lastref;
    s.expires = e->expires;
    s.lastmod = e->lastmod;
    s.swap_file_sz = e->swap_file_sz;
    s.refcount = e->refcount;
    s.flags = e->flags;
    xmemcpy(&s.key, e->key, MD5_DIGEST_CHARS);
    xmemcpy(state->outbuf + state->outbuf_offset, &s, ss);
    state->outbuf_offset += ss;
    /* buffered write */
    if (state->outbuf_offset + ss > CLEAN_BUF_SZ) {
	if (write(state->fd, state->outbuf, state->outbuf_offset) < 0) {
	    debug(50, 0) ("storeDirWriteCleanLogs: %s: write: %s\n",
		state->new, xstrerror());
	    debug(20, 0) ("storeDirWriteCleanLogs: Current swap logfile not replaced.\n");
	    file_close(state->fd);
	    state->fd = -1;
	    unlink(state->new);
	    safe_free(state);
	    sd->log.clean.state = NULL;
	    sd->log.clean.write = NULL;
	}
	state->outbuf_offset = 0;
    }
}

static void
storeUfsDirWriteCleanClose(SwapDir * sd)
{
    struct _clean_state *state = sd->log.clean.state;
    if (state->fd < 0)
	return;
    if (write(state->fd, state->outbuf, state->outbuf_offset) < 0) {
	debug(50, 0) ("storeDirWriteCleanLogs: %s: write: %s\n",
	    state->new, xstrerror());
	debug(20, 0) ("storeDirWriteCleanLogs: Current swap logfile "
	    "not replaced.\n");
	file_close(state->fd);
	state->fd = -1;
	unlink(state->new);
    }
    safe_free(state->outbuf);
    /*
     * You can't rename open files on Microsoft "operating systems"
     * so we have to close before renaming.
     */
    storeUfsDirCloseSwapLog(sd);
    /* rename */
    if (state->fd >= 0) {
#ifdef _SQUID_OS2_
	file_close(state->fd);
	state->fd = -1;
	if (unlink(cur) < 0)
	    debug(50, 0) ("storeDirWriteCleanLogs: unlinkd failed: %s, %s\n",
		xstrerror(), cur);
#endif
	xrename(state->new, state->cur);
    }
    /* touch a timestamp file if we're not still validating */
    if (store_dirs_rebuilding)
	(void) 0;
    else if (state->fd < 0)
	(void) 0;
    else
	file_close(file_open(state->cln, O_WRONLY | O_CREAT | O_TRUNC));
    /* close */
    safe_free(state->cur);
    safe_free(state->new);
    safe_free(state->cln);
    if (state->fd >= 0)
	file_close(state->fd);
    state->fd = -1;
    safe_free(state);
    sd->log.clean.state = NULL;
    sd->log.clean.write = NULL;
}

static void
storeUfsDirSwapLog(const SwapDir * sd, const StoreEntry * e, int op)
{
    storeSwapLogData *s = xcalloc(1, sizeof(storeSwapLogData));
    s->op = (char) op;
    s->swap_file_number = e->swap_file_number;
    s->timestamp = e->timestamp;
    s->lastref = e->lastref;
    s->expires = e->expires;
    s->lastmod = e->lastmod;
    s->swap_file_sz = e->swap_file_sz;
    s->refcount = e->refcount;
    s->flags = e->flags;
    xmemcpy(s->key, e->key, MD5_DIGEST_CHARS);
    file_write(sd->u.ufs.swaplog_fd,
	-1,
	s,
	sizeof(storeSwapLogData),
	NULL,
	NULL,
	xfree);
}

static void
storeUfsDirNewfs(SwapDir * sd)
{
    debug(47, 3) ("Creating swap space in %s\n", sd->path);
    storeUfsCreateDirectory(sd->path, 0);
    storeUfsCreateSwapSubDirs(sd);
}

static int
rev_int_sort(const void *A, const void *B)
{
    const int *i1 = A;
    const int *i2 = B;
    return *i2 - *i1;
}

static int
storeUfsDirClean(int swap_index)
{
    DIR *dp = NULL;
    struct dirent *de = NULL;
    LOCAL_ARRAY(char, p1, MAXPATHLEN + 1);
    LOCAL_ARRAY(char, p2, MAXPATHLEN + 1);
#if USE_TRUNCATE
    struct stat sb;
#endif
    int files[20];
    int swapfileno;
    int fn;			/* same as swapfileno, but with dirn bits set */
    int n = 0;
    int k = 0;
    int N0, N1, N2;
    int D0, D1, D2;
    N0 = n_ufs_dirs;
    D0 = ufs_dir_index[swap_index % N0];
    N1 = Config.cacheSwap.swapDirs[D0].u.ufs.l1;
    D1 = (swap_index / N0) % N1;
    N2 = Config.cacheSwap.swapDirs[D0].u.ufs.l2;
    D2 = ((swap_index / N0) / N1) % N2;
    snprintf(p1, SQUID_MAXPATHLEN, "%s/%02X/%02X",
	Config.cacheSwap.swapDirs[D0].path, D1, D2);
    debug(36, 3) ("storeDirClean: Cleaning directory %s\n", p1);
    dp = opendir(p1);
    if (dp == NULL) {
	if (errno == ENOENT) {
	    debug(36, 0) ("storeDirClean: WARNING: Creating %s\n", p1);
	    if (mkdir(p1, 0777) == 0)
		return 0;
	}
	debug(50, 0) ("storeDirClean: %s: %s\n", p1, xstrerror());
	safeunlink(p1, 1);
	return 0;
    }
    while ((de = readdir(dp)) != NULL && k < 20) {
	if (sscanf(de->d_name, "%X", &swapfileno) != 1)
	    continue;
	fn = storeDirProperFileno(D0, swapfileno);
	if (storeDirValidFileno(fn, 1))
	    if (storeDirMapBitTest(fn))
		if (storeUfsFilenoBelongsHere(fn, D0, D1, D2))
		    continue;
#if USE_TRUNCATE
	if (!stat(de->d_name, &sb))
	    if (sb.st_size == 0)
		continue;
#endif
	files[k++] = swapfileno;
    }
    closedir(dp);
    if (k == 0)
	return 0;
    qsort(files, k, sizeof(int), rev_int_sort);
    if (k > 10)
	k = 10;
    for (n = 0; n < k; n++) {
	debug(36, 3) ("storeDirClean: Cleaning file %08X\n", files[n]);
	snprintf(p2, MAXPATHLEN + 1, "%s/%08X", p1, files[n]);
#if USE_TRUNCATE
	truncate(p2, 0);
#else
	safeunlink(p2, 0);
#endif
	Counter.swap_files_cleaned++;
    }
    debug(36, 3) ("Cleaned %d unused files from %s\n", k, p1);
    return k;
}

static void
storeUfsDirCleanEvent(void *unused)
{
    static int swap_index = 0;
    int i;
    int j = 0;
    int n = 0;
    /*
     * Assert that there are UFS cache_dirs configured, otherwise
     * we should never be called.
     */
    assert(n_ufs_dirs);
    if (NULL == ufs_dir_index) {
	SwapDir *sd;
	/*
	 * Initialize the little array that translates UFS cache_dir
	 * number into the Config.cacheSwap.swapDirs array index.
	 */
	ufs_dir_index = xcalloc(n_ufs_dirs, sizeof(*ufs_dir_index));
	for (i = 0, n = 0; i < Config.cacheSwap.n_configured; i++) {
	    sd = &Config.cacheSwap.swapDirs[i];
	    if (!storeUfsDirIs(sd))
		continue;
	    ufs_dir_index[n++] = i;
	    j += (sd->u.ufs.l1 * sd->u.ufs.l2);
	}
	assert(n == n_ufs_dirs);
	/*
	 * Start the storeUfsDirClean() swap_index with a random
	 * value.  j equals the total number of UFS level 2
	 * swap directories
	 */
	swap_index = (int) (squid_random() % j);
    }
    if (0 == store_dirs_rebuilding) {
	n = storeUfsDirClean(swap_index);
	swap_index++;
    }
    eventAdd("storeDirClean", storeUfsDirCleanEvent, NULL,
	15.0 * exp(-0.25 * n), 1);
}

static int
storeUfsDirIs(SwapDir * sd)
{
    if (sd->type == SWAPDIR_UFS)
	return 1;
    if (sd->type == SWAPDIR_ASYNCUFS)
	return 1;
    return 0;
}

/*
 * Does swapfile number 'fn' belong in cachedir #F0,
 * level1 dir #F1, level2 dir #F2?
 *
 * Don't check that (fn >> SWAP_DIR_SHIFT) == F0 because
 * 'fn' may not have the directory bits set.
 */
static int
storeUfsFilenoBelongsHere(int fn, int F0, int F1, int F2)
{
    int D1, D2;
    int L1, L2;
    int filn = fn & SWAP_FILE_MASK;
    assert(F0 < Config.cacheSwap.n_configured);
    L1 = Config.cacheSwap.swapDirs[F0].u.ufs.l1;
    L2 = Config.cacheSwap.swapDirs[F0].u.ufs.l2;
    D1 = ((filn / L2) / L2) % L1;
    if (F1 != D1)
	return 0;
    D2 = (filn / L2) % L2;
    if (F2 != D2)
	return 0;
    return 1;
}

/* ========== LOCAL FUNCTIONS ABOVE, GLOBAL FUNCTIONS BELOW ========== */

void
storeUfsDirStats(StoreEntry * sentry)
{
    int i;
    SwapDir *SD;
#if HAVE_STATVFS
    struct statvfs sfs;
#endif
    for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	SD = &Config.cacheSwap.swapDirs[i];
	storeAppendPrintf(sentry, "\n");
	storeAppendPrintf(sentry, "Store Directory #%d: %s\n", i, SD->path);
	storeAppendPrintf(sentry, "First level subdirectories: %d\n", SD->u.ufs.l1);
	storeAppendPrintf(sentry, "Second level subdirectories: %d\n", SD->u.ufs.l2);
	storeAppendPrintf(sentry, "Maximum Size: %d KB\n", SD->max_size);
	storeAppendPrintf(sentry, "Current Size: %d KB\n", SD->cur_size);
	storeAppendPrintf(sentry, "Percent Used: %0.2f%%\n",
	    100.0 * SD->cur_size / SD->max_size);
	storeAppendPrintf(sentry, "Filemap bits in use: %d of %d (%d%%)\n",
	    SD->map->n_files_in_map, SD->map->max_n_files,
	    percent(SD->map->n_files_in_map, SD->map->max_n_files));
#if HAVE_STATVFS
#define fsbtoblk(num, fsbs, bs) \
        (((fsbs) != 0 && (fsbs) < (bs)) ? \
                (num) / ((bs) / (fsbs)) : (num) * ((fsbs) / (bs)))
	if (!statvfs(SD->path, &sfs)) {
	    storeAppendPrintf(sentry, "Filesystem Space in use: %d/%d KB (%d%%)\n",
		fsbtoblk((sfs.f_blocks - sfs.f_bfree), sfs.f_frsize, 1024),
		fsbtoblk(sfs.f_blocks, sfs.f_frsize, 1024),
		percent(sfs.f_blocks - sfs.f_bfree, sfs.f_blocks));
	    storeAppendPrintf(sentry, "Filesystem Inodes in use: %d/%d (%d%%)\n",
		sfs.f_files - sfs.f_ffree, sfs.f_files,
		percent(sfs.f_files - sfs.f_ffree, sfs.f_files));
	}
#endif
	storeAppendPrintf(sentry, "Flags:");
	if (SD->flags.selected)
	    storeAppendPrintf(sentry, " SELECTED");
	if (SD->flags.read_only)
	    storeAppendPrintf(sentry, " READ-ONLY");
	storeAppendPrintf(sentry, "\n");
    }
}

void
storeUfsDirParse(cacheSwap * swap)
{
    char *token;
    char *path;
    int i;
    int size;
    int l1;
    int l2;
    unsigned int read_only = 0;
    SwapDir *sd = NULL;
    if ((path = strtok(NULL, w_space)) == NULL)
	self_destruct();
    i = GetInteger();
    size = i << 10;		/* Mbytes to kbytes */
    if (size <= 0)
	fatal("storeUfsDirParse: invalid size value");
    i = GetInteger();
    l1 = i;
    if (l1 <= 0)
	fatal("storeUfsDirParse: invalid level 1 directories value");
    i = GetInteger();
    l2 = i;
    if (l2 <= 0)
	fatal("storeUfsDirParse: invalid level 2 directories value");
    if ((token = strtok(NULL, w_space)))
	if (!strcasecmp(token, "read-only"))
	    read_only = 1;
    for (i = 0; i < swap->n_configured; i++) {
	sd = swap->swapDirs + i;
	if (!strcmp(path, sd->path)) {
	    /* just reconfigure it */
	    if (size == sd->max_size)
		debug(3, 1) ("Cache dir '%s' size remains unchanged at %d KB\n",
		    path, size);
	    else
		debug(3, 1) ("Cache dir '%s' size changed to %d KB\n",
		    path, size);
	    sd->max_size = size;
	    if (sd->flags.read_only != read_only)
		debug(3, 1) ("Cache dir '%s' now %s\n",
		    path, read_only ? "Read-Only" : "Read-Write");
	    sd->flags.read_only = read_only;
	    return;
	}
    }
    allocate_new_swapdir(swap);
    sd = swap->swapDirs + swap->n_configured;
    sd->type = SWAPDIR_UFS;
    sd->index = swap->n_configured;
    sd->path = xstrdup(path);
    sd->max_size = size;
    sd->u.ufs.l1 = l1;
    sd->u.ufs.l2 = l2;
    sd->u.ufs.swaplog_fd = -1;
    sd->flags.read_only = read_only;
    sd->init = storeUfsDirInit;
    sd->newfs = storeUfsDirNewfs;
    sd->obj.open = storeUfsOpen;
    sd->obj.close = storeUfsClose;
    sd->obj.read = storeUfsRead;
    sd->obj.write = storeUfsWrite;
    sd->obj.unlink = storeUfsUnlink;
    sd->log.open = storeUfsDirOpenSwapLog;
    sd->log.close = storeUfsDirCloseSwapLog;
    sd->log.write = storeUfsDirSwapLog;
    sd->log.clean.open = storeUfsDirWriteCleanOpen;
    swap->n_configured++;
}

#if USE_ASYNC_IO
void
storeAufsDirParse(cacheSwap * swap)
{
    char *token;
    char *path;
    int i;
    int size;
    int l1;
    int l2;
    unsigned int read_only = 0;
    SwapDir *sd = NULL;
    if ((path = strtok(NULL, w_space)) == NULL)
	self_destruct();
    i = GetInteger();
    size = i << 10;		/* Mbytes to kbytes */
    if (size <= 0)
	fatal("storeUfsDirParse: invalid size value");
    i = GetInteger();
    l1 = i;
    if (l1 <= 0)
	fatal("storeUfsDirParse: invalid level 1 directories value");
    i = GetInteger();
    l2 = i;
    if (l2 <= 0)
	fatal("storeUfsDirParse: invalid level 2 directories value");
    if ((token = strtok(NULL, w_space)))
	if (!strcasecmp(token, "read-only"))
	    read_only = 1;
    for (i = 0; i < swap->n_configured; i++) {
	sd = swap->swapDirs + i;
	if (!strcmp(path, sd->path)) {
	    /* just reconfigure it */
	    if (size == sd->max_size)
		debug(3, 1) ("Cache dir '%s' size remains unchanged at %d KB\n",
		    path, size);
	    else
		debug(3, 1) ("Cache dir '%s' size changed to %d KB\n",
		    path, size);
	    sd->max_size = size;
	    if (sd->flags.read_only != read_only)
		debug(3, 1) ("Cache dir '%s' now %s\n",
		    path, read_only ? "Read-Only" : "Read-Write");
	    sd->flags.read_only = read_only;
	    return;
	}
    }
    allocate_new_swapdir(swap);
    sd = swap->swapDirs + swap->n_configured;
    sd->type = SWAPDIR_ASYNCUFS;
    sd->index = swap->n_configured;
    sd->path = xstrdup(path);
    sd->max_size = size;
    sd->u.ufs.l1 = l1;
    sd->u.ufs.l2 = l2;
    sd->u.ufs.swaplog_fd = -1;
    sd->flags.read_only = read_only;
    sd->init = storeUfsDirInit;
    sd->newfs = storeUfsDirNewfs;
    sd->obj.open = storeAufsOpen;
    sd->obj.close = storeAufsClose;
    sd->obj.read = storeAufsRead;
    sd->obj.write = storeAufsWrite;
    sd->obj.unlink = storeAufsUnlink;
    sd->log.open = storeUfsDirOpenSwapLog;
    sd->log.close = storeUfsDirCloseSwapLog;
    sd->log.write = storeUfsDirSwapLog;
    sd->log.clean.open = storeUfsDirWriteCleanOpen;
    swap->n_configured++;
}
#endif

void
storeUfsDirDump(StoreEntry * entry, const char *name, SwapDir * s)
{
    storeAppendPrintf(entry, "%s %s %s %d %d %d\n",
	name,
	SwapDirType[s->type],
	s->path,
	s->max_size >> 10,
	s->u.ufs.l1,
	s->u.ufs.l2);
}

/*
 * Only "free" the filesystem specific stuff here
 */
void
storeUfsDirFree(SwapDir * s)
{
    if (s->u.ufs.swaplog_fd > -1) {
	file_close(s->u.ufs.swaplog_fd);
	s->u.ufs.swaplog_fd = -1;
    }
}

char *
storeUfsFullPath(int fn, char *fullpath)
{
    LOCAL_ARRAY(char, fullfilename, SQUID_MAXPATHLEN);
    int dirn = (fn >> SWAP_DIR_SHIFT) % Config.cacheSwap.n_configured;
    int filn = fn & SWAP_FILE_MASK;
    SwapDir *SD = &Config.cacheSwap.swapDirs[dirn];
    int L1 = SD->u.ufs.l1;
    int L2 = SD->u.ufs.l2;
    if (!fullpath)
	fullpath = fullfilename;
    fullpath[0] = '\0';
    snprintf(fullpath, SQUID_MAXPATHLEN, "%s/%02X/%02X/%08X",
	Config.cacheSwap.swapDirs[dirn].path,
	((filn / L2) / L2) % L1,
	(filn / L2) % L2,
	filn);
    return fullpath;
}
