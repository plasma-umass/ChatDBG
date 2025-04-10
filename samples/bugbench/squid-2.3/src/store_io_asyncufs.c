
/*
 * DEBUG 78
 */

#include "squid.h"

#define SWAP_DIR_SHIFT 24
#define SWAP_FILE_MASK 0x00FFFFFF
#define MAGIC1 (NUMTHREADS/Config.cacheSwap.n_configured/2)

static AIOCB storeAufsReadDone;
static AIOCB storeAufsWriteDone;
static void storeAufsIOCallback(storeIOState * sio, int errflag);
static AIOCB storeAufsOpenDone;
static int storeAufsSomethingPending(storeIOState *);
static int storeAufsKickWriteQueue(storeIOState * sio);

struct _queued_write {
    char *buf;
    size_t size;
    off_t offset;
    FREE *free_func;
};

struct _queued_read {
    char *buf;
    size_t size;
    off_t offset;
    STRCB *callback;
    void *callback_data;
};

/* === PUBLIC =========================================================== */

storeIOState *
storeAufsOpen(sfileno f, mode_t mode, STIOCB * callback, void *callback_data)
{
    char *path = storeUfsFullPath(f, NULL);
    storeIOState *sio;
    debug(78, 3) ("storeAufsOpen: fileno %08X, mode %d\n", f, mode);
    assert(mode == O_RDONLY || mode == O_WRONLY);
    /*
     * we should detect some 'too many files open' condition and return
     * NULL here.
     */
    while (aioQueueSize() > MAGIC1)
	return NULL;
    sio = memAllocate(MEM_STORE_IO);
    cbdataAdd(sio, memFree, MEM_STORE_IO);
    sio->type.aufs.fd = -1;
    sio->swap_file_number = f;
    sio->mode = mode;
    sio->callback = callback;
    sio->callback_data = callback_data;
    cbdataLock(callback_data);
    if (mode == O_WRONLY)
	mode |= (O_CREAT | O_TRUNC);
    sio->type.aufs.flags.opening = 1;
    aioOpen(path, mode, 0644, storeAufsOpenDone, sio);
    Opening_FD++;
    store_open_disk_fd++;
    return sio;
}

void
storeAufsClose(storeIOState * sio)
{
    debug(78, 3) ("storeAufsClose: fileno %08X, FD %d\n",
	sio->swap_file_number, sio->type.aufs.fd);
    if (storeAufsSomethingPending(sio)) {
	sio->type.aufs.flags.close_request = 1;
	return;
    }
    storeAufsIOCallback(sio, DISK_OK);
}

void
storeAufsRead(storeIOState * sio, char *buf, size_t size, off_t offset, STRCB * callback, void *callback_data)
{
    assert(sio->read.callback == NULL);
    assert(sio->read.callback_data == NULL);
    assert(!sio->type.aufs.flags.reading);
    if (sio->type.aufs.fd < 0) {
	struct _queued_read *q;
	debug(78, 3) ("storeAufsRead: queueing read because FD < 0\n");
	assert(sio->type.aufs.flags.opening);
	assert(sio->type.aufs.pending_reads == NULL);
	q = xcalloc(1, sizeof(*q));
	q->buf = buf;
	q->size = size;
	q->offset = offset;
	q->callback = callback;
	q->callback_data = callback_data;
	linklistPush(&sio->type.aufs.pending_reads, q);
	return;
    }
    sio->read.callback = callback;
    sio->read.callback_data = callback_data;
    sio->type.aufs.read_buf = buf;
    cbdataLock(callback_data);
    debug(78, 3) ("storeAufsRead: fileno %08X, FD %d\n",
	sio->swap_file_number, sio->type.aufs.fd);
    sio->offset = offset;
    sio->type.aufs.flags.reading = 1;
    aioRead(sio->type.aufs.fd, offset, buf, size, storeAufsReadDone, sio);
}

void
storeAufsWrite(storeIOState * sio, char *buf, size_t size, off_t offset, FREE * free_func)
{
    debug(78, 3) ("storeAufsWrite: fileno %08X, FD %d\n",
	sio->swap_file_number, sio->type.aufs.fd);
    if (sio->type.aufs.fd < 0) {
	/* disk file not opened yet */
	struct _queued_write *q;
	assert(sio->type.aufs.flags.opening);
	q = xcalloc(1, sizeof(*q));
	q->buf = buf;
	q->size = size;
	q->offset = offset;
	q->free_func = free_func;
	linklistPush(&sio->type.aufs.pending_writes, q);
	return;
    }
    if (sio->type.aufs.flags.writing) {
	struct _queued_write *q;
	debug(78, 3) ("storeAufsWrite: queuing write\n");
	q = xcalloc(1, sizeof(*q));
	q->buf = buf;
	q->size = size;
	q->offset = offset;
	q->free_func = free_func;
	linklistPush(&sio->type.aufs.pending_writes, q);
	return;
    }
    sio->type.aufs.flags.writing = 1;
    /*
     * XXX it might be nice if aioWrite() gave is immediate
     * feedback here about EWOULDBLOCK instead of in the
     * callback function
     */
    aioWrite(sio->type.aufs.fd,
	offset,
	buf,
	size,
	storeAufsWriteDone,
	sio,
	free_func);
}

void
storeAufsUnlink(sfileno f)
{
    debug(78, 3) ("storeAufsUnlink: fileno %08X\n", f);
    aioUnlink(storeUfsFullPath(f, NULL), NULL, NULL);
}

/*  === STATIC =========================================================== */

static int
storeAufsKickWriteQueue(storeIOState * sio)
{
    struct _queued_write *q = linklistShift(&sio->type.aufs.pending_writes);
    if (NULL == q)
	return 0;
    debug(78, 3) ("storeAufsKickWriteQueue: writing queued chunk of %d bytes\n",
	q->size);
    storeAufsWrite(sio, q->buf, q->size, q->offset, q->free_func);
    xfree(q);
    return 1;
}

static int
storeAufsKickReadQueue(storeIOState * sio)
{
    struct _queued_read *q = linklistShift(&sio->type.aufs.pending_reads);
    if (NULL == q)
	return 0;
    debug(78, 3) ("storeAufsKickReadQueue: reading queued request of %d bytes\n",
	q->size);
    storeAufsRead(sio, q->buf, q->size, q->offset, q->callback, q->callback_data);
    xfree(q);
    return 1;
}

static void
storeAufsOpenDone(int unused, void *my_data, int fd, int errflag)
{
    storeIOState *sio = my_data;
    debug(78, 3) ("storeAufsOpenDone: FD %d, errflag %d\n", fd, errflag);
    Opening_FD--;
    sio->type.aufs.flags.opening = 0;
    if (errflag || fd < 0) {
	errno = errflag;
	debug(78, 0) ("storeAufsOpenDone: %s\n", xstrerror());
	debug(78, 1) ("\t%s\n", storeUfsFullPath(sio->swap_file_number, NULL));
	storeAufsIOCallback(sio, DISK_ERROR);
	return;
    }
    sio->type.aufs.fd = fd;
    commSetCloseOnExec(fd);
    fd_open(fd, FD_FILE, storeUfsFullPath(sio->swap_file_number, NULL));
    if (sio->mode == O_WRONLY)
	storeAufsKickWriteQueue(sio);
    else if (sio->mode == O_RDONLY)
	storeAufsKickReadQueue(sio);
    debug(78, 3) ("storeAufsOpenDone: exiting\n");
}

static void
storeAufsReadDone(int fd, void *my_data, int len, int errflag)
{
    storeIOState *sio = my_data;
    STRCB *callback = sio->read.callback;
    void *their_data = sio->read.callback_data;
    ssize_t rlen;
    debug(78, 3) ("storeAufsReadDone: fileno %08X, FD %d, len %d\n",
	sio->swap_file_number, fd, len);
    sio->type.aufs.flags.reading = 0;
    if (errflag) {
	debug(78, 3) ("storeAufsReadDone: got failure (%d)\n", errflag);
	rlen = -1;
    } else {
	rlen = (ssize_t) len;
	sio->offset += len;
    }
    assert(callback);
    assert(their_data);
    sio->read.callback = NULL;
    sio->read.callback_data = NULL;
    if (cbdataValid(their_data))
	callback(their_data, sio->type.aufs.read_buf, rlen);
    cbdataUnlock(their_data);
    /* 
     * XXX is this safe?  The above callback may have caused sio
     * to be freed/closed already? Philip Guenther <guenther@gac.edu>
     * says it fixes his FD leaks, with no side effects.
     */
    if (sio->type.aufs.flags.close_request)
	storeAufsIOCallback(sio, DISK_OK);
}

/*
 * XXX TODO
 * if errflag == EWOULDBLOCK, then we'll need to re-queue the
 * chunk at the beginning of the write_pending list and try
 * again later.
 */
static void
storeAufsWriteDone(int fd, void *my_data, int len, int errflag)
{
    static int loop_detect = 0;
    storeIOState *sio = my_data;
    debug(78, 3) ("storeAufsWriteDone: fileno %08X, FD %d, len %d\n",
	sio->swap_file_number, fd, len);
    assert(++loop_detect < 10);
    sio->type.aufs.flags.writing = 0;
    if (errflag) {
	debug(78, 3) ("storeAufsWriteDone: got failure (%d)\n", errflag);
	storeAufsIOCallback(sio, DISK_ERROR);
	loop_detect--;
	return;
    }
    sio->offset += len;
    if (storeAufsKickWriteQueue(sio))
	(void) 0;
    else if (sio->type.aufs.flags.close_request)
	storeAufsIOCallback(sio, DISK_OK);
    loop_detect--;
}

static void
storeAufsIOCallback(storeIOState * sio, int errflag)
{
    STIOCB *callback = sio->callback;
    void *their_data = sio->callback_data;
    int fd = sio->type.aufs.fd;
    debug(78, 3) ("storeAufsIOCallback: errflag=%d\n", errflag);
    sio->callback = NULL;
    sio->callback_data = NULL;
    if (callback)
	if (NULL == their_data || cbdataValid(their_data))
	    callback(their_data, errflag, sio);
    cbdataUnlock(their_data);
    sio->type.aufs.fd = -1;
    cbdataFree(sio);
    if (fd < 0)
	return;
    aioClose(fd);
    fd_close(fd);
    store_open_disk_fd--;
}


static int
storeAufsSomethingPending(storeIOState * sio)
{
    if (sio->type.aufs.flags.reading)
	return 1;
    if (sio->type.aufs.flags.writing)
	return 1;
    if (sio->type.aufs.flags.opening)
	return 1;
    return 0;
}
