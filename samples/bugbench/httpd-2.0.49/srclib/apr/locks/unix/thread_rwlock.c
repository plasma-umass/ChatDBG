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

#include "apr_arch_thread_rwlock.h"
#include "apr_private.h"

#if APR_HAS_THREADS

#ifdef HAVE_PTHREAD_RWLOCKS

static apr_status_t thread_rwlock_cleanup(void *data)
{
    apr_thread_rwlock_t *rwlock = (apr_thread_rwlock_t *)data;
    apr_status_t stat;

    pthread_rwlock_unlock(rwlock->rwlock);
    stat = pthread_rwlock_destroy(rwlock->rwlock);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    return stat;
} 

APR_DECLARE(apr_status_t) apr_thread_rwlock_create(apr_thread_rwlock_t **rwlock,
                                                   apr_pool_t *pool)
{
    apr_thread_rwlock_t *new_rwlock;
    apr_status_t stat;

    new_rwlock = (apr_thread_rwlock_t *)apr_pcalloc(pool,
                                                  sizeof(apr_thread_rwlock_t));

    if (new_rwlock == NULL) {
        return APR_ENOMEM;
    }

    new_rwlock->pool = pool;
    new_rwlock->rwlock = (pthread_rwlock_t *)apr_palloc(pool, 
                                                     sizeof(pthread_rwlock_t));

    if (new_rwlock->rwlock == NULL) {
        return APR_ENOMEM;
    }

    if ((stat = pthread_rwlock_init(new_rwlock->rwlock, NULL))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        thread_rwlock_cleanup(new_rwlock);
        return stat;
    }

    apr_pool_cleanup_register(new_rwlock->pool,
                              (void *)new_rwlock, thread_rwlock_cleanup,
                              apr_pool_cleanup_null);

    *rwlock = new_rwlock;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_rdlock(apr_thread_rwlock_t *rwlock)
{
    apr_status_t stat;

    stat = pthread_rwlock_rdlock(rwlock->rwlock);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    return stat;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_tryrdlock(apr_thread_rwlock_t *rwlock)
{
    apr_status_t stat;

    stat = pthread_rwlock_tryrdlock(rwlock->rwlock);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    /* Normalize the return code. */
    if (stat == EBUSY)
        stat = APR_EBUSY;
    return stat;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_wrlock(apr_thread_rwlock_t *rwlock)
{
    apr_status_t stat;

    stat = pthread_rwlock_wrlock(rwlock->rwlock);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    return stat;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_trywrlock(apr_thread_rwlock_t *rwlock)
{
    apr_status_t stat;

    stat = pthread_rwlock_trywrlock(rwlock->rwlock);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    /* Normalize the return code. */
    if (stat == EBUSY)
        stat = APR_EBUSY;
    return stat;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_unlock(apr_thread_rwlock_t *rwlock)
{
    apr_status_t stat;

    stat = pthread_rwlock_unlock(rwlock->rwlock);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    return stat;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_destroy(apr_thread_rwlock_t *rwlock)
{
    apr_status_t stat;
    if ((stat = thread_rwlock_cleanup(rwlock)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(rwlock->pool, rwlock, thread_rwlock_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

#else  /* HAVE_PTHREAD_RWLOCKS */

APR_DECLARE(apr_status_t) apr_thread_rwlock_create(apr_thread_rwlock_t **rwlock,
                                                   apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_rdlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_tryrdlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_wrlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_trywrlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_unlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_destroy(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

#endif /* HAVE_PTHREAD_RWLOCKS */
APR_POOL_IMPLEMENT_ACCESSOR(thread_rwlock)

#endif /* APR_HAS_THREADS */
