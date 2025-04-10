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

#include "apr.h"
#include "apr_poll.h"
#include "apr_arch_networkio.h"
#include "apr_arch_file_io.h"
#if HAVE_POLL_H
#include <poll.h>
#endif
#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

APR_DECLARE(apr_status_t) apr_poll_setup(apr_pollfd_t **new, apr_int32_t num, apr_pool_t *cont)
{
    (*new) = (apr_pollfd_t *)apr_pcalloc(cont, sizeof(apr_pollfd_t) * (num + 1));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)[num].desc_type = APR_POLL_LASTDESC;
    (*new)[0].p = cont;
    return APR_SUCCESS;
}

static apr_pollfd_t *find_poll_sock(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    apr_pollfd_t *curr = aprset;
    
    while (curr->desc.s != sock) {
        if (curr->desc_type == APR_POLL_LASTDESC) {
            return NULL;
        }
        curr++;
    }

    return curr;
}

APR_DECLARE(apr_status_t) apr_poll_socket_add(apr_pollfd_t *aprset, 
			       apr_socket_t *sock, apr_int16_t event)
{
    apr_pollfd_t *curr = aprset;
    
    while (curr->desc_type != APR_NO_DESC) {
        if (curr->desc_type == APR_POLL_LASTDESC) {
            return APR_ENOMEM;
        }
        curr++;
    }
    curr->desc.s = sock;
    curr->desc_type = APR_POLL_SOCKET;
    curr->reqevents = event;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_revents_get(apr_int16_t *event, apr_socket_t *sock, apr_pollfd_t *aprset)
{
    apr_pollfd_t *curr = find_poll_sock(aprset, sock);
    if (curr == NULL) {
        return APR_NOTFOUND;
    }

    (*event) = curr->rtnevents;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_mask(apr_pollfd_t *aprset, 
                                  apr_socket_t *sock, apr_int16_t events)
{
    apr_pollfd_t *curr = find_poll_sock(aprset, sock);
    if (curr == NULL) {
        return APR_NOTFOUND;
    }
    
    if (curr->reqevents & events) {
        curr->reqevents ^= events;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_remove(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    apr_pollfd_t *match = NULL;
    apr_pollfd_t *curr;

    for (curr = aprset; (curr->desc_type != APR_POLL_LASTDESC) &&
             (curr->desc_type != APR_NO_DESC); curr++) {
        if (curr->desc.s == sock) {
            match = curr;
        }
    }
    if (match == NULL) {
        return APR_NOTFOUND;
    }

    /* Remove this entry by swapping the last entry into its place.
     * This ensures that the non-APR_NO_DESC entries are all at the
     * start of the array, so that apr_poll() doesn't have to worry
     * about invalid entries in the middle of the pollset.
     */
    curr--;
    if (curr != match) {
        *match = *curr;
    }
    curr->desc_type = APR_NO_DESC;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_clear(apr_pollfd_t *aprset, apr_int16_t events)
{
    apr_pollfd_t *curr = aprset;

    while (curr->desc_type != APR_POLL_LASTDESC) {
        if (curr->reqevents & events) {
            curr->reqevents &= ~events;
        }
        curr++;
    }
    return APR_SUCCESS;
}

#if APR_FILES_AS_SOCKETS
/* I'm not sure if this needs to return an apr_status_t or not, but
 * for right now, we'll leave it this way, and change it later if
 * necessary.
 */
APR_DECLARE(apr_status_t) apr_socket_from_file(apr_socket_t **newsock, apr_file_t *file)
{
    (*newsock) = apr_pcalloc(file->pool, sizeof(**newsock));
    (*newsock)->socketdes = file->filedes;
    (*newsock)->cntxt = file->pool;
    (*newsock)->timeout = file->timeout;
    return APR_SUCCESS;
}
#endif
