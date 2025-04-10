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
#include "apr_atomic.h"
#include "apr_thread_mutex.h"

#if !defined(apr_atomic_init) && !defined(APR_OVERRIDE_ATOMIC_INIT)

#if APR_HAS_THREADS
#define NUM_ATOMIC_HASH 7
/* shift by 2 to get rid of alignment issues */
#define ATOMIC_HASH(x) (unsigned int)(((unsigned long)(x)>>2)%(unsigned int)NUM_ATOMIC_HASH)
static apr_thread_mutex_t **hash_mutex;
#endif /* APR_HAS_THREADS */

apr_status_t apr_atomic_init(apr_pool_t *p)
{
#if APR_HAS_THREADS
    int i;
    apr_status_t rv;
    hash_mutex = apr_palloc(p, sizeof(apr_thread_mutex_t*) * NUM_ATOMIC_HASH);

    for (i = 0; i < NUM_ATOMIC_HASH; i++) {
        rv = apr_thread_mutex_create(&(hash_mutex[i]),
                                     APR_THREAD_MUTEX_DEFAULT, p);
        if (rv != APR_SUCCESS) {
           return rv;
        }
    }
#endif /* APR_HAS_THREADS */
    return APR_SUCCESS;
}
#endif /*!defined(apr_atomic_init) && !defined(APR_OVERRIDE_ATOMIC_INIT) */

#if !defined(apr_atomic_add) && !defined(APR_OVERRIDE_ATOMIC_ADD)
void apr_atomic_add(volatile apr_atomic_t *mem, apr_uint32_t val) 
{
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    apr_uint32_t prev;
       
    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        *mem += val;
        apr_thread_mutex_unlock(lock);
    }
#else
    *mem += val;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_add) && !defined(APR_OVERRIDE_ATOMIC_ADD) */

#if !defined(apr_atomic_set) && !defined(APR_OVERRIDE_ATOMIC_SET)
void apr_atomic_set(volatile apr_atomic_t *mem, apr_uint32_t val) 
{
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    apr_uint32_t prev;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        *mem = val;
        apr_thread_mutex_unlock(lock);
    }
#else
    *mem = val;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_set) && !defined(APR_OVERRIDE_ATOMIC_SET) */

#if !defined(apr_atomic_inc) && !defined(APR_OVERRIDE_ATOMIC_INC)
void apr_atomic_inc(volatile apr_uint32_t *mem) 
{
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    apr_uint32_t prev;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        (*mem)++;
        apr_thread_mutex_unlock(lock);
    }
#else
    (*mem)++;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_inc) && !defined(APR_OVERRIDE_ATOMIC_INC) */

#if !defined(apr_atomic_dec) && !defined(APR_OVERRIDE_ATOMIC_DEC)
int apr_atomic_dec(volatile apr_atomic_t *mem) 
{
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    apr_uint32_t new;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        (*mem)--;
        new = *mem;
        apr_thread_mutex_unlock(lock);
        return new; 
    }
#else
    (*mem)--;
#endif /* APR_HAS_THREADS */
    return *mem; 
}
#endif /*!defined(apr_atomic_dec) && !defined(APR_OVERRIDE_ATOMIC_DEC) */

#if !defined(apr_atomic_cas) && !defined(APR_OVERRIDE_ATOMIC_CAS)
apr_uint32_t apr_atomic_cas(volatile apr_uint32_t *mem, long with, long cmp)
{
    long prev;
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *(long*)mem;
        if (prev == cmp) {
            *(long*)mem = with;
        }
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *(long*)mem;
#else
    prev = *(long*)mem;
    if (prev == cmp) {
        *(long*)mem = with;
    }
    return prev;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_cas) && !defined(APR_OVERRIDE_ATOMIC_CAS) */

#if !defined(apr_atomic_casptr) && !defined(APR_OVERRIDE_ATOMIC_CASPTR)
void *apr_atomic_casptr(volatile void **mem, void *with, const void *cmp)
{
    void *prev;
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *(void **)mem;
        if (prev == cmp) {
            *mem = with;
        }
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *(void **)mem;
#else
    prev = *(void **)mem;
    if (prev == cmp) {
        *mem = with;
    }
    return prev;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_cas) && !defined(APR_OVERRIDE_ATOMIC_CAS) */
