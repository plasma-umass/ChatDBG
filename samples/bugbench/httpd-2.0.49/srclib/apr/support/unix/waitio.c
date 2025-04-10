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

#include "apr_arch_file_io.h"
#include "apr_arch_networkio.h"
#include "apr_poll.h"
#include "apr_errno.h"
#include "apr_support.h"

/* The only case where we don't use wait_for_io_or_timeout is on
 * pre-BONE BeOS, so this check should be sufficient and simpler */
#if !BEOS_R5
#define USE_WAIT_FOR_IO
#endif

#ifdef USE_WAIT_FOR_IO
apr_status_t apr_wait_for_io_or_timeout(apr_file_t *f, apr_socket_t *s,
                                           int for_read)
{
    apr_interval_time_t timeout;
    apr_pollfd_t pollset;
    int srv, n;
    int type = for_read ? APR_POLLIN : APR_POLLOUT;

    /* TODO - timeout should be less each time through this loop */
    if (f) {
        pollset.desc_type = APR_POLL_FILE;
        pollset.desc.f = f;
        pollset.p = f->pool;
        timeout = f->timeout;
    }
    else {
        pollset.desc_type = APR_POLL_SOCKET;
        pollset.desc.s = s;
        pollset.p = s->cntxt;
        timeout = s->timeout;
    }
    pollset.reqevents = type;

    do {
        srv = apr_poll(&pollset, 1, &n, timeout);

        if (n == 1 && pollset.rtnevents & type) {
            return APR_SUCCESS;
        }
    } while (APR_STATUS_IS_EINTR(srv));

    return srv;
}
#endif

