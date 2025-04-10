
/*
 * $Id: store_io_ufs.c,v 1.11.2.4 2000/02/09 23:30:02 wessels Exp $
 *
 * DEBUG: section 79    Storage Manager UFS Interface
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

#define SWAP_DIR_SHIFT 24
#define SWAP_FILE_MASK 0x00FFFFFF

static DRCB storeUfsReadDone;
static DWCB storeUfsWriteDone;
static void storeUfsIOCallback(storeIOState * sio, int errflag);

/* === PUBLIC =========================================================== */

storeIOState *
storeUfsOpen(sfileno f, mode_t mode, STIOCB * callback, void *callback_data)
{
    char *path = storeUfsFullPath(f, NULL);
    storeIOState *sio;
    struct stat sb;
    int fd;
    debug(79, 3) ("storeUfsOpen: fileno %08X, mode %d\n", f, mode);
    assert(mode == O_RDONLY || mode == O_WRONLY);
    if (mode == O_WRONLY)
	mode |= (O_CREAT | O_TRUNC);
    fd = file_open(path, mode);
    if (fd < 0) {
	debug(79, 3) ("storeUfsOpenDone: got failure (%d)\n", errno);
	return NULL;
    }
    debug(79, 3) ("storeUfsOpen: opened FD %d\n", fd);
    sio = memAllocate(MEM_STORE_IO);
    cbdataAdd(sio, memFree, MEM_STORE_IO);
    sio->swap_file_number = f;
    sio->mode = mode;
    sio->callback = callback;
    sio->callback_data = callback_data;
    sio->type.ufs.fd = fd;
    sio->type.ufs.flags.writing = 0;
    if (sio->mode == O_RDONLY)
	if (fstat(fd, &sb) == 0)
	    sio->st_size = sb.st_size;
    store_open_disk_fd++;
    return sio;
}

void
storeUfsClose(storeIOState * sio)
{
    debug(79, 3) ("storeUfsClose: fileno %08X, FD %d\n",
	sio->swap_file_number, sio->type.ufs.fd);
    if (sio->type.ufs.flags.reading || sio->type.ufs.flags.writing) {
	sio->type.ufs.flags.close_request = 1;
	return;
    }
    storeUfsIOCallback(sio, 0);
}

void
storeUfsRead(storeIOState * sio, char *buf, size_t size, off_t offset, STRCB * callback, void *callback_data)
{
    assert(sio->read.callback == NULL);
    assert(sio->read.callback_data == NULL);
    sio->read.callback = callback;
    sio->read.callback_data = callback_data;
    cbdataLock(callback_data);
    debug(79, 3) ("storeUfsRead: fileno %08X, FD %d\n",
	sio->swap_file_number, sio->type.ufs.fd);
    sio->offset = offset;
    sio->type.ufs.flags.reading = 1;
    file_read(sio->type.ufs.fd,
	buf,
	size,
	offset,
	storeUfsReadDone,
	sio);
}

void
storeUfsWrite(storeIOState * sio, char *buf, size_t size, off_t offset, FREE * free_func)
{
    debug(79, 3) ("storeUfsWrite: fileno %08X, FD %d\n", sio->swap_file_number, sio->type.ufs.fd);
    sio->type.ufs.flags.writing = 1;
    file_write(sio->type.ufs.fd,
	offset,
	buf,
	size,
	storeUfsWriteDone,
	sio,
	free_func);
}

void
storeUfsUnlink(sfileno f)
{
    debug(79, 3) ("storeUfsUnlink: fileno %08X\n", f);
    unlinkdUnlink(storeUfsFullPath(f, NULL));
}

/*  === STATIC =========================================================== */

static void
storeUfsReadDone(int fd, const char *buf, int len, int errflag, void *my_data)
{
    storeIOState *sio = my_data;
    STRCB *callback = sio->read.callback;
    void *their_data = sio->read.callback_data;
    debug(79, 3) ("storeUfsReadDone: fileno %08X, FD %d, len %d\n",
	sio->swap_file_number, fd, len);
    sio->type.ufs.flags.reading = 0;
    if (errflag) {
	debug(79, 3) ("storeUfsReadDone: got failure (%d)\n", errflag);
	storeUfsIOCallback(sio, errflag);
	return;
    }
    sio->offset += len;
    assert(callback);
    assert(their_data);
    sio->read.callback = NULL;
    sio->read.callback_data = NULL;
    if (cbdataValid(their_data))
	callback(their_data, buf, (size_t) len);
    cbdataUnlock(their_data);
}

static void
storeUfsWriteDone(int fd, int errflag, size_t len, void *my_data)
{
    storeIOState *sio = my_data;
    debug(79, 3) ("storeUfsWriteDone: fileno %08X, FD %d, len %d\n",
	sio->swap_file_number, fd, len);
    sio->type.ufs.flags.writing = 0;
    if (errflag) {
	debug(79, 0) ("storeUfsWriteDone: got failure (%d)\n", errflag);
	storeUfsIOCallback(sio, errflag);
	return;
    }
    sio->offset += len;
    if (sio->type.ufs.flags.close_request)
	storeUfsIOCallback(sio, errflag);
}

static void
storeUfsIOCallback(storeIOState * sio, int errflag)
{
    debug(79, 3) ("storeUfsIOCallback: errflag=%d\n", errflag);
    if (sio->type.ufs.fd > -1) {
	file_close(sio->type.ufs.fd);
	store_open_disk_fd--;
    }
    sio->callback(sio->callback_data, errflag, sio);
    cbdataFree(sio);
}
